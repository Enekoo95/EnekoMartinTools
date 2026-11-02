#include "CubeOscillator.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

ACubeOscillator::ACubeOscillator()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACubeOscillator::BeginPlay()
{
	Super::BeginPlay();
	CollectActors();
	CollectInstances();
}

void ACubeOscillator::CollectActors() 
{
	Cubes.Reset();
	BasePositions.Reset();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PerlinCube")))
		{
			Cubes.Add(*It);
			BasePositions.Add(It->GetActorLocation());
		}
	}
}

void ACubeOscillator::CollectInstances() 
{
	ISM = nullptr;
	BaseTransforms.Reset();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PerlinCubesISM")))
		{
			ISM = It->FindComponentByClass<UInstancedStaticMeshComponent>();
		}
	}
	if (!ISM) { return; }

	ISM->SetMobility(EComponentMobility::Movable);

	for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
	{
		FTransform T;
		ISM->GetInstanceTransform(i, T);
		BaseTransforms.Add(T);
	}
}

void ACubeOscillator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TRACE_CPUPROFILER_EVENT_SCOPE(CubeOscillator_Tick);

	Time += DeltaSeconds;

	auto WaveHeight = [this](const FVector& B)
		{
			return FMath::Sin(Time * Speed + (B.X + B.Y) * 0.01f) * Amplitude;
		};

	for (int32 i = 0; i < Cubes.Num(); ++i)
	{
		Cubes[i]->SetActorLocation(BasePositions[i] + FVector(0.f, 0.f, WaveHeight(BasePositions[i])));
	}

	if (!ISM) { return; }

	TArray<FTransform> NewTransforms = BaseTransforms;
	for (FTransform& T : NewTransforms)
	{
		T.AddToTranslation(FVector(0.f, 0.f, WaveHeight(T.GetLocation())));
	}
	ISM->BatchUpdateInstancesTransforms(0, NewTransforms, false, true);
}