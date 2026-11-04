#include "CityBlueprintFunctionLibrary.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UPCGComponent* UCityBlueprintFunctionLibrary::FindPCGComponent()
{
	if (!GEditor) { return nullptr; }

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) { return nullptr; }

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("PerlinPCG")))
		{
			return It->FindComponentByClass<UPCGComponent>();
		}
	}
	return nullptr;
}

void UCityBlueprintFunctionLibrary::ApplyPCGParameters(
	int32 Width, int32 Height, double Spacing,
	double NoiseSize, double Threshold, int32 Seed)
{
	UPCGComponent* PCG = FindPCGComponent();
	if (!PCG)
	{
		UE_LOG(LogTemp, Warning, TEXT("No se encontro un actor con tag PerlinPCG y PCGComponent"));
		return;
	}
	if (!PCG->GetGraphInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("El PCGComponent no tiene grafo asignado"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Aplicando PCG: W=%d H=%d Spacing=%f"), Width, Height, Spacing);

	UPCGGraphInstance* Graph = PCG->GetGraphInstance();

	const FTransform NoiseTransform(FQuat::Identity, FVector::ZeroVector, FVector(NoiseSize));

	Graph->SetGraphParameter<FVector>(TEXT("GridExtents"), FVector(Width, Height, 0.0) * Spacing * 0.5);
	Graph->SetGraphParameter<FVector>(TEXT("CellSize"), FVector(Spacing, Spacing, 100.0));
	Graph->SetGraphParameter<FTransform>(TEXT("NoiseTransform"), NoiseTransform);
	Graph->SetGraphParameter<double>(TEXT("Threshold"), Threshold);

	PCG->Seed = Seed;
	PCG->GenerateLocal(/*bForce=*/ true);


}