#include "FireFlamesComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// The values from MakeFireScene.cs, in centimetres.
	constexpr int32 MaxParticles = 400;
	constexpr float BaseRate = 80.f;
	constexpr float LifetimeMin = 0.6f, LifetimeMax = 1.2f;
	constexpr float SpeedMin = 120.f, SpeedMax = 240.f;
	constexpr float SizeMin = 35.f, SizeMax = 80.f;
	constexpr float Buoyancy = 0.15f * 981.f;   // gravityModifier = -0.15
	constexpr float ConeRadius = 35.f;
	constexpr float ConeAngleDegrees = 12.f;
	constexpr float PlaneMeshSize = 100.f;      // /Engine/BasicShapes/Plane is 100 cm wide

	// Colours in the sample are display values (its project is in gamma colour space); the renderer here is linear.
	FVector3f ToLinear(float R, float G, float B)
	{
		const FLinearColor C = FLinearColor::FromSRGBColor(FColor(FMath::RoundToInt(R * 255.f), FMath::RoundToInt(G * 255.f), FMath::RoundToInt(B * 255.f)));
		return FVector3f(C.R, C.G, C.B);
	}

	// colorOverLifetime: three colour keys...
	FVector3f ColorOverLife(float T)
	{
		static const FVector3f K0 = ToLinear(1.f, 0.9f, 0.4f), K1 = ToLinear(1.f, 0.45f, 0.05f), K2 = ToLinear(0.6f, 0.1f, 0.02f);
		return T < 0.4f ? FMath::Lerp(K0, K1, T / 0.4f) : FMath::Lerp(K1, K2, (T - 0.4f) / 0.6f);
	}

	// ...and four alpha keys.
	float AlphaOverLife(float T)
	{
		if (T < 0.1f) return T / 0.1f;
		if (T < 0.6f) return FMath::Lerp(1.f, 0.6f, (T - 0.1f) / 0.5f);
		return FMath::Lerp(0.6f, 0.f, (T - 0.6f) / 0.4f);
	}
}

UFireFlamesComponent::UFireFlamesComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;   // after the camera has been set

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Fire/M_FireFlame.M_FireFlame"));
	if (Plane.Succeeded()) SetStaticMesh(Plane.Object);
	if (Material.Succeeded()) SetMaterial(0, Material.Object);

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);
	SetMobility(EComponentMobility::Movable);
	SetCanEverAffectNavigation(false);
	bAffectDistanceFieldLighting = false;
	NumCustomDataFloats = 5;   // linear r, g, b, alpha, seed
}

void UFireFlamesComponent::BeginPlay()
{
	Super::BeginPlay();
	Particles.SetNum(MaxParticles);
	InstanceTransforms.Init(FTransform(FQuat::Identity, GetComponentLocation(), FVector(0.0001f)), MaxParticles);
	PreAllocateInstancesMemory(MaxParticles);
	AddInstances(InstanceTransforms, false, true);
}

int32 UFireFlamesComponent::NumAlive() const
{
	int32 Alive = 0;
	for (const FParticle& P : Particles) Alive += P.Lifetime > 0.f ? 1 : 0;
	return Alive;
}

void UFireFlamesComponent::Emit(int32 Count)
{
	for (int32 i = 0; i < Count; i++) Spawn();
}

void UFireFlamesComponent::Spawn()
{
	FParticle* Slot = Particles.FindByPredicate([](const FParticle& P) { return P.Lifetime <= 0.f; });
	if (!Slot) return;   // maxParticles reached, as in Unity

	// A cone with a 35 cm base, opening by 12 degrees towards its rim; its axis is up.
	const float Angle = FMath::FRand() * UE_TWO_PI;
	const float Radial = FMath::Sqrt(FMath::FRand());   // uniform over the disc
	const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	const float Tilt = FMath::DegreesToRadians(ConeAngleDegrees) * Radial;
	const FVector Direction = Out * FMath::Sin(Tilt) + FVector::UpVector * FMath::Cos(Tilt);

	Slot->Position = GetComponentLocation() + Out * Radial * ConeRadius;
	Slot->Velocity = Direction * FMath::FRandRange(SpeedMin, SpeedMax);
	Slot->Age = 0.f;
	Slot->Lifetime = FMath::FRandRange(LifetimeMin, LifetimeMax);
	Slot->StartSize = FMath::FRandRange(SizeMin, SizeMax) * SizeMultiplier;
	Slot->Roll = FMath::FRandRange(-0.35f, 0.35f);
	Slot->Seed = FMath::FRand();
	static const FVector3f ColorA = ToLinear(1.f, 0.55f, 0.1f), ColorB = ToLinear(1.f, 0.85f, 0.3f);
	Slot->StartColor = FMath::Lerp(ColorA, ColorB, FMath::FRand());
}

void UFireFlamesComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DeltaTime = FMath::Min(DeltaTime, 0.1f);

	if (bEmitting)
	{
		EmitDebt += BaseRate * RateMultiplier * DeltaTime;
		const int32 Count = FMath::FloorToInt(EmitDebt);
		EmitDebt -= Count;
		Emit(Count);
	}

	FVector CameraLocation = GetComponentLocation() + FVector(-380.f, 0.f, 120.f);
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager) CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	}

	float Custom[5];
	for (int32 i = 0; i < Particles.Num(); i++)
	{
		FParticle& P = Particles[i];
		if (P.Lifetime > 0.f)
		{
			P.Age += DeltaTime;
			if (P.Age >= P.Lifetime) P.Lifetime = 0.f;
		}
		if (P.Lifetime <= 0.f)
		{
			InstanceTransforms[i].SetScale3D(FVector(0.0001f));
			Custom[0] = Custom[1] = Custom[2] = Custom[3] = 0.f;
			Custom[4] = 0.f;
			SetCustomData(i, TArrayView<const float>(Custom, 5), false);
			continue;
		}

		P.Velocity.Z += Buoyancy * DeltaTime;
		P.Position += P.Velocity * DeltaTime;

		const float T = P.Age / P.Lifetime;
		const float Size = P.StartSize * FMath::Lerp(1.f, 0.2f, T);   // sizeOverLifetime
		const FVector3f Color = ColorOverLife(T) * P.StartColor;

		// The plane's normal (+Z) faces the camera. Its V runs along +Y, so -Y is world up and the flame stands upright;
		// each quad is a little taller than wide, the same area as Unity's square particle.
		const FVector ToCamera = (CameraLocation - P.Position).GetSafeNormal();
		const FQuat Facing = FRotationMatrix::MakeFromZY(ToCamera, -FVector::UpVector).ToQuat() * FQuat(FVector::UpVector, P.Roll);
		InstanceTransforms[i] = FTransform(Facing, P.Position, FVector(Size * 0.8f / PlaneMeshSize, Size * 1.25f / PlaneMeshSize, 1.f));

		Custom[0] = Color.X;
		Custom[1] = Color.Y;
		Custom[2] = Color.Z;
		Custom[3] = AlphaOverLife(T);
		Custom[4] = P.Seed;
		SetCustomData(i, TArrayView<const float>(Custom, 5), false);
	}

	BatchUpdateInstancesTransforms(0, InstanceTransforms, true, true, true);
}
