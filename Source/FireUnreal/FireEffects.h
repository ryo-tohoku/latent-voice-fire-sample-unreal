// AFireEffects: how the fire looks and sounds. Mirrors Assets/Fire/FireEffects.cs in the Unity Fire sample. Nothing here knows about voice.
// Light() starts the fire; called again while it burns, the fire flares up and grows. PutOut() ends it.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireEffects.generated.h"

class UAudioComponent;
class UFireFlamesComponent;
class UPointLightComponent;
class USoundWave;

UCLASS()
class FIREUNREAL_API AFireEffects : public AActor
{
	GENERATED_BODY()

public:
	AFireEffects();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void Light();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void PutOut();

	/** 0 when out; 1 when lit; one more each time Light is called while it burns, up to MaxSize. */
	int32 GetSize() const { return Size; }

	UPROPERTY(EditAnywhere, Category = "Fire", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxSize = 3;

	/** One line with the values the demo mode logs: glow, sound, particles. */
	FString DescribeState() const;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Apply();

	UFUNCTION()
	void OnSoundEnvelope(const USoundWave* Wave, const float Envelope);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFireFlamesComponent> Flames;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAudioComponent> Sound;

	int32 Size = 0;
	float BaseGlow = 1.f;
	float BaseVolume = 1.f;
	float Volume = 0.f;
	float Flash = 0.f;
	float EnvelopePeak = 0.f;   // the loudest the audio mixer rendered this sound since the last DescribeState
};
