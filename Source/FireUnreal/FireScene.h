// The campfire scene, built in code to match the Unity Fire sample's scene, and the temporary game framework
// around it: the voice hookup on the Latent Voice plugin, and the on-screen guide.
// FireEffects and FireOnWord are what a game takes from this sample.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "FireScene.generated.h"

class AFireEffects;
class AFireOnWord;
class ULatentVoiceSubsystem;
class SWidget;
class UCameraComponent;
class UDirectionalLightComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/** Ground, 4 crossed logs, 10 stones, the Unity default directional light, the background and the fixed camera. */
UCLASS()
class FIREUNREAL_API AFireScene : public AActor
{
	GENERATED_BODY()

public:
	AFireScene();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;

private:
	UStaticMeshComponent* AddPart(FName Name, UStaticMesh* Mesh);
	void Paint(UStaticMeshComponent* Part, FLinearColor DisplayColor, float Roughness);
	void PaintAll();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Background;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Logs;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Stones;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SolidMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BackgroundMaterial;
};

UCLASS()
class FIREUNREAL_API AFirePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFirePlayerController();

	void Connect(AFireEffects* InEffects, AFireOnWord* InOnWord) { Effects = InEffects; OnWord = InOnWord; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

private:

	void StartVoice();
	bool VoiceOn() const;   // false without the plugin (WITH_LATENT_VOICE 0)
	void OnVoiceWord(const FString& Word);

	void BuildGuide();
	void TickVoice(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AFireEffects> Effects;

	UPROPERTY()
	TObjectPtr<AFireOnWord> OnWord;

	UPROPERTY()
	TObjectPtr<UObject> FontRegular;

	UPROPERTY()
	TObjectPtr<UObject> FontBold;

	UPROPERTY()
	TObjectPtr<UObject> FontJapanese;

	TSharedPtr<SWidget> Guide;

	// Voice, from the plugin. Null without the plugin.
	TWeakObjectPtr<ULatentVoiceSubsystem> Voice;
	FDelegateHandle WordHandle;
	float LastWordScore = -1.f;      // of the last word; negative when it came from a key
	float MeterLevel = 0.f;
	float MeterPeak = 0.f;
	double MeterPeakAt = 0.0;
	double VoiceListeningSince = -1.0;
};

UCLASS()
class FIREUNREAL_API AFireGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFireGameMode();
	virtual void StartPlay() override;
};
