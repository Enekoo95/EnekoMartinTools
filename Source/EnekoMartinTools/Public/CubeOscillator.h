#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeOscillator.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class ENEKOMARTINTOOLS_API ACubeOscillator : public AActor
{
	GENERATED_BODY()

public:
	ACubeOscillator();
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Oscillator") float Amplitude = 50.f;
	UPROPERTY(EditAnywhere, Category = "Oscillator") float Speed = 2.f;

private:
	void CollectActors();
	void CollectInstances();

	UPROPERTY() TArray<TObjectPtr<AActor>> Cubes;
	TArray<FVector> BasePositions;

	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ISM;
	TArray<FTransform> BaseTransforms;

	float Time = 0.f;
};