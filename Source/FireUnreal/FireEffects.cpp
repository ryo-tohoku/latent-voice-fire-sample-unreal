#include "FireEffects.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "FireFlamesComponent.h"
#include "FireUnreal.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

AFireEffects::AFireEffects()
{
	PrimaryActorTick.bCanEverTick = true;

	Flames = CreateDefaultSubobject<UFireFlamesComponent>(TEXT("Flames"));
	SetRootComponent(Flames);

	// Unity: point light at y = 0.9 m, colour (1, 0.55, 0.2), range 6 m, intensity 3, no shadows. This actor stands at
	// y = 0.3 m. The falloff exponent and intensity are matched by eye and arithmetic to Unity's attenuation curve.
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Flames);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Glow->SetMobility(EComponentMobility::Movable);
	Glow->bUseInverseSquaredFalloff = false;
	Glow->LightFalloffExponent = 6.f;
	Glow->AttenuationRadius = 600.f;
	Glow->Intensity = 8.f;
	Glow->LightColor = FColor(255, 140, 51);
	Glow->CastShadows = false;
	Glow->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<USoundWave> Loop(TEXT("/Game/Fire/FireLoop.FireLoop"));
	Sound = CreateDefaultSubobject<UAudioComponent>(TEXT("Sound"));
	Sound->SetupAttachment(Flames);
	Sound->bAutoActivate = false;
	Sound->bAllowSpatialization = false;   // spatialBlend = 0
	if (Loop.Succeeded()) Sound->SetSound(Loop.Object);
}

void AFireEffects::BeginPlay()
{
	Super::BeginPlay();
	BaseGlow = Glow->Intensity;
	BaseVolume = 1.f;
	Sound->OnAudioSingleEnvelopeValue.AddDynamic(this, &AFireEffects::OnSoundEnvelope);
	Apply();
}

void AFireEffects::Light()
{
	if (Size == 0)
	{
		Size = 1;
		UE_LOG(LogFire, Log, TEXT("[Fire sample] the fire is lit"));
		Flames->Play();
	}
	else
	{
		Size = FMath::Min(Size + 1, MaxSize);
		UE_LOG(LogFire, Log, TEXT("[Fire sample] the fire flares up (size %d/%d)"), Size, MaxSize);
		// Unity plays an optional flare sound here; the sample ships none, so neither does this.
	}
	Apply();
	Flames->Emit(40 * Size);   // a burst, so repeating the word always shows
	Flash = 1.f;
}

void AFireEffects::PutOut()
{
	UE_LOG(LogFire, Log, TEXT("[Fire sample] the fire is out"));
	Size = 0;
	Flames->StopEmitting();
	Apply();
}

// Each size step makes the flames 35 % larger and denser.
void AFireEffects::Apply()
{
	const float Scale = 1.f + 0.35f * FMath::Max(0, Size - 1);
	Flames->SizeMultiplier = Scale;
	Flames->RateMultiplier = Scale;
	Glow->SetVisibility(Size > 0);
	if (Size > 0 && !Sound->IsPlaying())
	{
		Volume = 0.f;
		Sound->SetVolumeMultiplier(0.f);
		Sound->Play();
	}
}

// The glow and the sound follow the size, with a short flash after each word; the sound fades out with the flames.
void AFireEffects::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Flash = FMath::FInterpConstantTo(Flash, 0.f, DeltaSeconds, 2.f);
	if (Size > 0)
	{
		Glow->SetIntensity(BaseGlow * (1.f + 0.35f * (Size - 1)) * (1.f + 1.5f * Flash));
	}
	if (Sound->IsPlaying())
	{
		const float Target = Size == 0 ? 0.f : BaseVolume * (0.6f + 0.2f * (Size - 1)) * (1.f + 0.5f * Flash);
		Volume = FMath::FInterpConstantTo(Volume, FMath::Min(1.f, Target), DeltaSeconds, Size == 0 ? 0.8f : 3.f);
		Sound->SetVolumeMultiplier(Volume);
		if (Size == 0 && Volume <= 0.f)
		{
			Sound->Stop();
			UE_LOG(LogFire, Log, TEXT("[Fire sample] the sound has faded out and stopped"));
		}
	}
}

void AFireEffects::OnSoundEnvelope(const USoundWave* Wave, const float Envelope)
{
	EnvelopePeak = FMath::Max(EnvelopePeak, Envelope);
}

FString AFireEffects::DescribeState() const
{
	AFireEffects* Self = const_cast<AFireEffects*>(this);
	const float Peak = EnvelopePeak;
	Self->EnvelopePeak = 0.f;
	return FString::Printf(TEXT("size=%d/%d glow.visible=%d glow.intensity=%.2f flash=%.2f particles.alive=%d sizeMult=%.2f rateMult=%.2f sound.playing=%d sound.volume=%.3f sound.mixerEnvelopePeak=%.4f"),
		Size, MaxSize, Glow->IsVisible() ? 1 : 0, Glow->Intensity, Flash, Flames->NumAlive(), Flames->SizeMultiplier, Flames->RateMultiplier,
		Sound->IsPlaying() ? 1 : 0, Volume, Peak);
}
