#include "FireScene.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "FireEffects.h"
#include "FireOnWord.h"
#include "FireUnreal.h"
#include "Fonts/CompositeFont.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
// The Latent Voice plugin is optional: the project opens and plays without it, and the guide says how to add it (FireUnreal.Build.cs sets
// WITH_LATENT_VOICE from whether Plugins/LatentVoice is there).
#if WITH_LATENT_VOICE
#include "LatentVoiceSession.h"
#include "LatentVoiceSubsystem.h"
#endif
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UnrealClient.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

// Unity (left-handed, Y up, metres) to Unreal (left-handed, Z up, centimetres).
static FVector FromUnity(float X, float Y, float Z) { return FVector(Z * 100.f, X * 100.f, Y * 100.f); }

// The Unity sample's project is in gamma colour space, so its colours are display values.
static FLinearColor DisplayToLinear(float R, float G, float B)
{
	return FLinearColor(FMath::Pow(R, 2.2f), FMath::Pow(G, 2.2f), FMath::Pow(B, 2.2f));
}

// Values matched to the look Unity's numbers imply. Unity has ambient from its default skybox, which has
// no equivalent here without a sky, so the solid material adds albedo * AmbientLight as emissive.
static const float SunLux = 3.0f;
static const float AmbientLight = 0.25f;
static const float ManualExposureBias = 0.f;
static const FLinearColor BackgroundEmissive(0.0045f, 0.0045f, 0.009f);   // calibrated: displays as (0.06, 0.06, 0.09)

///////////////////////////////////////////////////////////////////////////////
// AFireScene

AFireScene::AFireScene()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Solid(TEXT("/Game/Fire/M_FireSolid.M_FireSolid"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Back(TEXT("/Game/Fire/M_FireBackground.M_FireBackground"));
	SolidMaterial = Solid.Object;
	BackgroundMaterial = Back.Object;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// A Unity plane is 10 m wide, scaled by 3; the Unreal plane is 1 m wide.
	Ground = AddPart(TEXT("Ground"), PlaneMesh.Object);
	Ground->SetRelativeScale3D(FVector(30.f, 30.f, 1.f));

	// No sky: a large unlit sphere in the camera's clear colour.
	Background = AddPart(TEXT("Background"), SphereMesh.Object);
	Background->SetRelativeScale3D(FVector(400.f));
	Background->SetMaterial(0, BackgroundMaterial);
	Background->SetCastShadow(false);

	// Logs: a Unity cylinder is 2 m long and 1 m wide, scaled to 1 m by 14 cm; the Unreal one is 1 m by 1 m.
	static const float Yaw[] = { 0.f, 90.f, 45.f, 135.f };
	for (int32 i = 0; i < 4; i++)
	{
		UStaticMeshComponent* Log = AddPart(*FString::Printf(TEXT("Log%d"), i), CylinderMesh.Object);
		Log->SetRelativeLocation(FromUnity(0.f, i < 2 ? 0.1f : 0.26f, 0.f));
		Log->SetRelativeRotation(FRotator(90.f, Yaw[i], 0.f));   // lying down, its axis at this yaw
		Log->SetRelativeScale3D(FVector(0.14f, 0.14f, 1.f));
		Logs.Add(Log);
	}

	for (int32 i = 0; i < 10; i++)
	{
		const float A = i * UE_TWO_PI / 10.f;
		UStaticMeshComponent* Stone = AddPart(*FString::Printf(TEXT("Stone%d"), i), SphereMesh.Object);
		Stone->SetRelativeLocation(FromUnity(FMath::Cos(A) * 0.75f, 0.06f, FMath::Sin(A) * 0.75f));
		Stone->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.2f));
		Stones.Add(Stone);
	}

	// The directional light from Unity's default scene: Euler (50, -30, 0), colour (1, 0.957, 0.839), soft shadows.
	const float Pitch = FMath::DegreesToRadians(50.f), Heading = FMath::DegreesToRadians(-30.f);
	const FVector UnityForward(FMath::Sin(Heading) * FMath::Cos(Pitch), -FMath::Sin(Pitch), FMath::Cos(Heading) * FMath::Cos(Pitch));
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(GetRootComponent());
	Sun->SetRelativeRotation(FRotationMatrix::MakeFromX(FVector(UnityForward.Z, UnityForward.X, UnityForward.Y)).Rotator());
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->Intensity = SunLux;
	Sun->LightColor = FColor(255, 244, 214);
	Sun->CastShadows = true;
	Sun->DynamicShadowDistanceMovableLight = 3000.f;

	// Unity: position (0, 1.5, -3.8), looking at (0, 0.45, 0), vertical field of view 60.
	const FVector CameraAt = FromUnity(0.f, 1.5f, -3.8f);
	const FVector LookAt = FromUnity(0.f, 0.45f, 0.f);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());
	Camera->SetRelativeLocation(CameraAt);
	Camera->SetRelativeRotation((LookAt - CameraAt).Rotation());
	// Unreal's field of view is horizontal at AspectRatio; this pair is a vertical 60 at any window shape.
	Camera->AspectRatio = 16.f / 9.f;
	Camera->SetFieldOfView(FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(30.f)) * 16.f / 9.f)));
	Camera->bConstrainAspectRatio = false;
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->AspectRatioAxisConstraint = AspectRatio_MaintainYFOV;

	FPostProcessSettings& Post = Camera->PostProcessSettings;
	Post.bOverride_AutoExposureMethod = true;
	Post.AutoExposureMethod = AEM_Manual;
	Post.bOverride_AutoExposureBias = true;
	Post.AutoExposureBias = ManualExposureBias;
	Post.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Post.AutoExposureApplyPhysicalCameraExposure = 0;
	Post.bOverride_MotionBlurAmount = true;
	Post.MotionBlurAmount = 0.f;
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = 0.f;
	Post.bOverride_FilmToe = true;
	Post.FilmToe = 0.4f;   // lifts the dark ground towards Unity's untonemapped values
	Post.bOverride_BloomIntensity = true;
	Post.BloomIntensity = 0.35f;
}

UStaticMeshComponent* AFireScene::AddPart(FName Name, UStaticMesh* Mesh)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetRootComponent());
	Part->SetStaticMesh(Mesh);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	return Part;
}

void AFireScene::Paint(UStaticMeshComponent* Part, FLinearColor LinearColor, float Roughness)
{
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(SolidMaterial, this);
	M->SetFlags(RF_Transient);   // made again on load and at Play, so never saved into the level
	M->SetVectorParameterValue(TEXT("Color"), LinearColor);
	M->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	M->SetScalarParameterValue(TEXT("Ambient"), AmbientLight);
	Part->SetMaterial(0, M);
}

// Also in the Editor, so the placed scene in FireMap shows its colours before Play.
void AFireScene::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	PaintAll();
#if WITH_EDITORONLY_DATA
	// A level opens in the Editor at the view saved in it. One that never had a view (FireMap as the generator makes it) gets the game's camera,
	// so a first look at the level is the campfire, not the inside of a stone. A view the developer saves later is kept.
	UWorld* World = GetWorld();
	if (World && !World->IsGameWorld() && World->EditorViews.IsValidIndex(ELevelViewportType::LVT_Perspective))
	{
		FLevelViewportInfo& View = World->EditorViews[ELevelViewportType::LVT_Perspective];
		if (View.CamPosition.IsNearlyZero() && View.CamRotation.IsNearlyZero())
		{
			View = FLevelViewportInfo(Camera->GetComponentLocation(), Camera->GetComponentRotation(), View.CamOrthoZoom);
		}
	}
#endif
}

void AFireScene::BeginPlay()
{
	Super::BeginPlay();
	PaintAll();
}

void AFireScene::PaintAll()
{
	// Samples~/Fire/Materials: _Color and, as roughness, 1 - _Glossiness.
	Paint(Ground, DisplayToLinear(0.22f, 0.2f, 0.18f), 0.9f);
	for (UStaticMeshComponent* Log : Logs) Paint(Log, DisplayToLinear(0.36f, 0.22f, 0.12f), 0.85f);
	for (UStaticMeshComponent* Stone : Stones) Paint(Stone, DisplayToLinear(0.45f, 0.45f, 0.47f), 0.75f);

	UMaterialInstanceDynamic* Back = UMaterialInstanceDynamic::Create(BackgroundMaterial, this);
	Back->SetFlags(RF_Transient);
	Back->SetVectorParameterValue(TEXT("Color"), BackgroundEmissive);
	Background->SetMaterial(0, Back);
}

///////////////////////////////////////////////////////////////////////////////
// AFirePlayerController

AFirePlayerController::AFirePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bAutoManageActiveCameraTarget = false;
	bShowMouseCursor = true;

	// Engine font faces: nothing added to the project. Droid Sans Fallback has the kana for 「ファイア」 and 「ストップ」.
	static ConstructorHelpers::FObjectFinder<UObject> Regular(TEXT("/Engine/EngineFonts/Faces/RobotoRegular.RobotoRegular"));
	static ConstructorHelpers::FObjectFinder<UObject> Bold(TEXT("/Engine/EngineFonts/Faces/RobotoBold.RobotoBold"));
	static ConstructorHelpers::FObjectFinder<UObject> Japanese(TEXT("/Engine/EngineFonts/Faces/DroidSansFallback.DroidSansFallback"));
	FontRegular = Regular.Object;
	FontBold = Bold.Object;
	FontJapanese = Japanese.Object;
}

void AFirePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AFirePlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	BuildGuide();

	StartVoice();
}

// The plugin does the listening. This starts it and acts on the words it reports.
void AFirePlayerController::StartVoice()
{
#if WITH_LATENT_VOICE
	Voice = ULatentVoiceSubsystem::Get(this);
	if (!Voice.IsValid())
	{
		UE_LOG(LogFire, Error, TEXT("[Voice] the Latent Voice subsystem is not there: is the plugin enabled?"));
		return;
	}
	FLatentVoiceOptions Options;
	WordHandle = Voice->OnKeywordDetectedNative.AddUObject(this, &AFirePlayerController::OnVoiceWord);
	Voice->StartListeningWithOptions(Options);
#else
	UE_LOG(LogFire, Warning, TEXT("[Voice] the Latent Voice plugin is not in this project yet: see the guide on the screen"));
#endif
}

bool AFirePlayerController::VoiceOn() const
{
#if WITH_LATENT_VOICE
	return Voice.IsValid();
#else
	return false;
#endif
}


void AFirePlayerController::OnVoiceWord(const FString& Word)
{
	if (!OnWord) return;
	OnWord->OnWord(Word);
#if WITH_LATENT_VOICE
	LastWordScore = Voice.IsValid() ? Voice->GetLastKeywordScore() : -1.f;
#endif
	UE_LOG(LogFire, Log, TEXT("[Voice] 「%s」 reached OnWord (score %.2f); fire size now %d"), *Word, LastWordScore, Effects ? Effects->GetSize() : -1);
}


void AFirePlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Guide.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Guide.ToSharedRef());
	}
	Guide.Reset();
#if WITH_LATENT_VOICE
	if (Voice.IsValid())
	{
		Voice->OnKeywordDetectedNative.Remove(WordHandle);
		Voice->StopListening();
		Voice.Reset();
	}
#endif
	Super::EndPlay(Reason);
}

void AFirePlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (VoiceOn()) TickVoice(DeltaSeconds);
}

void AFirePlayerController::TickVoice(float DeltaSeconds)
{
#if WITH_LATENT_VOICE
	const double Now = GetWorld()->GetRealTimeSeconds();

	// The meter: rises at once, falls over about a third of a second; the peak mark holds for a second. Unity's InputLevel scale.
	const float Level = Voice->GetInputLevel();
	MeterLevel = Level >= MeterLevel ? Level : FMath::Max(Level, MeterLevel - DeltaSeconds * 3.f);
	if (Level >= MeterPeak || Now - MeterPeakAt > 1.0)
	{
		MeterPeak = Level;
		MeterPeakAt = Now;
	}

	if (VoiceListeningSince < 0.0 && Voice->IsListening())
	{
		VoiceListeningSince = Now;
		UE_LOG(LogFire, Log, TEXT("[Voice] listening for %s; status: %s"), *FString::Join(Voice->GetKeywords(), TEXT(", ")), *Voice->GetStatus());
	}
#endif
}

void AFirePlayerController::BuildGuide()
{
	if (!GEngine || !GEngine->GameViewport) return;

	auto MakeFont = [this](UObject* Latin, int32 Points)
	{
		TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>();
		FTypefaceEntry& Default = Font->DefaultTypeface.Fonts.Emplace_GetRef(FName("Regular"));
		Default.Font = FFontData(Latin);
		FTypefaceEntry& Fallback = Font->FallbackTypeface.Typeface.Fonts.Emplace_GetRef(FName("Regular"));
		Fallback.Font = FFontData(FontJapanese);
		return FSlateFontInfo(TSharedPtr<const FCompositeFont>(Font), Points, FName("Regular"));
	};
	const FSlateFontInfo TitleFont = MakeFont(FontBold, 20);
	const FSlateFontInfo LineFont = MakeFont(FontRegular, 15);
	const FSlateFontInfo LastFont = MakeFont(FontBold, 15);

	TWeakObjectPtr<AFirePlayerController> Self(this);
	static const float MeterWidth = 560.f;

	Guide = SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.f)
		[
			SNew(SBox).WidthOverride(660.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.65f))
				.Padding(FMargin(20.f, 14.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock).Font(TitleFont).ColorAndOpacity(FLinearColor::White)
						.Text(FText::FromString(TEXT("Latent Voice: Fire sample")))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock).Font(LineFont).AutoWrapText(true)
						.ColorAndOpacity_Lambda([Self]()
						{
							if (!Self.IsValid() || !Self->VoiceOn()) return FSlateColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0xD1, 0x66)));
#if WITH_LATENT_VOICE
							switch (Self->Voice->GetVoiceState())
							{
							case ELatentVoiceState::Listening: return FSlateColor(FLinearColor::FromSRGBColor(FColor(0x7D, 0xFD, 0x99)));
							case ELatentVoiceState::Failed:
							case ELatentVoiceState::Stopped: return FSlateColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0x6B, 0x5B)));
							default: return FSlateColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0xD1, 0x66)));
							}
#else
							return FSlateColor(FLinearColor::FromSRGBColor(FColor(0xFF, 0xD1, 0x66)));
#endif
						})
						.Text_Lambda([Self]()
						{
#if WITH_LATENT_VOICE
							if (!Self.IsValid() || !Self->VoiceOn()) return FText::FromString(TEXT("● Voice is not running."));
							return FText::FromString(FString(TEXT("● ")) + Self->Voice->GetStatus());
#else
							return FText::FromString(TEXT("● The Latent Voice plugin is not in this project yet. Close the Editor, put the LatentVoice folder from the developer portal into Plugins/, and open the project again: the Editor offers to build it."));
#endif
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 6.f)
					[
						// The mic meter: grey when quiet, green at speaking volume, orange near clipping. FireSampleGuide.cs's bar.
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
						[
							SNew(STextBlock).Font(LineFont).ColorAndOpacity(FLinearColor::White).Text(FText::FromString(TEXT("Mic")))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SBox).HeightOverride(14.f).WidthOverride(MeterWidth)
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								[
									SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.15f))
								]
								+ SOverlay::Slot().HAlign(HAlign_Left)
								[
									SNew(SBox)
									.WidthOverride_Lambda([Self]() { return FOptionalSize(Self.IsValid() && Self->VoiceOn() ? MeterWidth * Self->MeterLevel : 0.f); })
									[
										SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
										.BorderBackgroundColor_Lambda([Self]()
										{
											const float V = Self.IsValid() && Self->VoiceOn() ? Self->MeterLevel : 0.f;
											return FSlateColor(V < 0.35f ? FLinearColor::FromSRGBColor(FColor(0x99, 0x99, 0x99)) : V < 0.85f ? FLinearColor::FromSRGBColor(FColor(0x7D, 0xFD, 0x99)) : FLinearColor::FromSRGBColor(FColor(0xFF, 0x99, 0x33)));
										})
									]
								]
								+ SOverlay::Slot().HAlign(HAlign_Left)
								[
									SNew(SBox)
									.WidthOverride_Lambda([Self]() { return FOptionalSize(Self.IsValid() && Self->VoiceOn() ? MeterWidth * Self->MeterPeak : 0.f); })
									.HAlign(HAlign_Right)
									[
										SNew(SBox).WidthOverride(2.f)
										[
											SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor::White)
										]
									]
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock).Font(LineFont)
						.ColorAndOpacity_Lambda([Self]()
						{
							const bool bLit = Self.IsValid() && Self->Effects && Self->Effects->GetSize() > 0;
							return FSlateColor(bLit ? FLinearColor::FromSRGBColor(FColor(0xFF, 0x9A, 0x3C)) : FLinearColor::FromSRGBColor(FColor(0xBB, 0xBB, 0xBB)));
						})
						.Text_Lambda([Self]()
						{
							if (!Self.IsValid() || !Self->Effects) return FText::GetEmpty();
							const int32 Size = Self->Effects->GetSize();
							return FText::FromString(Size == 0 ? FString(TEXT("● Fire: not burning"))
								: FString::Printf(TEXT("● Fire: burning (size %d/%d)"), Size, Self->Effects->MaxSize));
						})
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock).Font(LineFont).ColorAndOpacity(FLinearColor::White).AutoWrapText(true)
						.Text_Lambda([Self]()
						{
							if (!Self.IsValid() || !Self->OnWord) return FText::GetEmpty();
							return FText::FromString(FString::Printf(TEXT("Say 「%s」 to light the fire (again to make it bigger), 「%s」 to put it out."),
								*Self->OnWord->FireWord, *Self->OnWord->StopWord));
						})
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(LastFont)
						.ColorAndOpacity_Lambda([Self]()
						{
							const bool bFresh = Self.IsValid() && Self->OnWord && Self->OnWord->GetLastWordTime() >= 0.0
								&& Self->GetWorld()->GetRealTimeSeconds() - Self->OnWord->GetLastWordTime() < 1.5;
							return FSlateColor(bFresh ? FLinearColor::FromSRGBColor(FColor(0xFF, 0xD1, 0x66)) : FLinearColor::FromSRGBColor(FColor(0xBB, 0xBB, 0xBB)));
						})
						.Text_Lambda([Self]()
						{
							if (!Self.IsValid() || !Self->OnWord || Self->OnWord->GetLastWord().IsEmpty()) return FText::FromString(TEXT("Last word: none yet"));
							return FText::FromString(Self->LastWordScore >= 0.f
								? FString::Printf(TEXT("Last word: 「%s」 (score %.2f)"), *Self->OnWord->GetLastWord(), Self->LastWordScore)
								: FString::Printf(TEXT("Last word: 「%s」"), *Self->OnWord->GetLastWord()));
						})
					]
				]
			]
		];

	GEngine->GameViewport->AddViewportWidgetContent(Guide.ToSharedRef(), 10);
}


///////////////////////////////////////////////////////////////////////////////
// AFireGameMode

AFireGameMode::AFireGameMode()
{
	PlayerControllerClass = AFirePlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AFireGameMode::StartPlay()
{
	// FireMap holds the three actors, placed as in the Unity scene, so they can be seen and selected in the Editor.
	// A level without them still works: each missing one is spawned here.
	UWorld* World = GetWorld();
	TActorIterator<AFireScene> PlacedScene(World);
	TActorIterator<AFireEffects> PlacedEffects(World);
	TActorIterator<AFireOnWord> PlacedOnWord(World);
	AFireScene* Scene = PlacedScene ? *PlacedScene : World->SpawnActor<AFireScene>();

	// Unity: the fire at (0, 0.3, 0), on the logs.
	AFireEffects* Effects = PlacedEffects ? *PlacedEffects : World->SpawnActor<AFireEffects>(FromUnity(0.f, 0.3f, 0.f), FRotator::ZeroRotator);
	AFireOnWord* OnWord = PlacedOnWord ? *PlacedOnWord : World->SpawnActor<AFireOnWord>();
	if (!OnWord->Effects) OnWord->Effects = Effects;
	UE_LOG(LogFire, Log, TEXT("[Fire sample] scene: %s %s, fire: %s %s, words: %s %s"),
		PlacedScene ? TEXT("placed") : TEXT("spawned"), *Scene->GetActorNameOrLabel(),
		PlacedEffects ? TEXT("placed") : TEXT("spawned"), *Effects->GetActorNameOrLabel(),
		PlacedOnWord ? TEXT("placed") : TEXT("spawned"), *OnWord->GetActorNameOrLabel());

	if (AFirePlayerController* PC = Cast<AFirePlayerController>(World->GetFirstPlayerController()))
	{
		PC->Connect(Effects, OnWord);
		PC->SetViewTarget(Scene);
	}
	Super::StartPlay();
}
