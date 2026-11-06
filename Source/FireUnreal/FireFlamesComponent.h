// UFireFlamesComponent: the flames, as a small CPU particle simulation drawn as additive camera-facing quads.
// Same parameters as the flames ParticleSystem in the Unity Fire sample. Plays the role of ParticleSystem
// for AFireEffects: Play, StopEmitting, Emit, and multipliers for size and rate.
#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "FireFlamesComponent.generated.h"

UCLASS()
class FIREUNREAL_API UFireFlamesComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UFireFlamesComponent();

	void Play() { bEmitting = true; }
	void StopEmitting() { bEmitting = false; }
	void Emit(int32 Count);

	float SizeMultiplier = 1.f;
	float RateMultiplier = 1.f;

	int32 NumAlive() const;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FParticle
	{
		FVector Position = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FVector3f StartColor = FVector3f::OneVector;
		float Age = 0.f;
		float Lifetime = 0.f;   // 0: free slot
		float StartSize = 0.f;
		float Roll = 0.f;
		float Seed = 0.f;
	};

	void Spawn();

	TArray<FParticle> Particles;
	TArray<FTransform> InstanceTransforms;
	bool bEmitting = false;
	float EmitDebt = 0.f;
};
