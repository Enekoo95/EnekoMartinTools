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
	FVector GridExtent, FVector CellSize, double NoiseSize, double Threshold, int32 Seed)
{
	UPCGComponent* PCG = FindPCGComponent();
	if (!PCG)
	{
		UE_LOG(LogTemp, Warning, TEXT("No se encontro un actor con tag PerlinPCG"));
		return;
	}
	if (!PCG->GetGraphInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("El PCGComponent no tiene grafo asignado"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Aplicando PCG: GridExtent=%s CellSize=%s NoiseSize=%f Threshold=%f Seed=%d"),
		*GridExtent.ToString(), *CellSize.ToString(), NoiseSize, Threshold, Seed);

	UPCGGraphInstance* Graph = PCG->GetGraphInstance();
	const FTransform NoiseXform(FQuat::Identity, FVector::ZeroVector, FVector(NoiseSize));

	auto Check = [](EPropertyBagResult R, const TCHAR* Name)
		{
			if (R != EPropertyBagResult::Success)
			{
				UE_LOG(LogTemp, Warning, TEXT("No se pudo asignar el parametro %s (nombre o tipo incorrecto)"), Name);
			}
		};

	Check(Graph->SetGraphParameter<FVector>(TEXT("GridExtents"), GridExtent), TEXT("GridExtents"));
	Check(Graph->SetGraphParameter<FVector>(TEXT("CellSize"), CellSize), TEXT("CellSize"));
	Check(Graph->SetGraphParameter<FTransform>(TEXT("NoiseTransform"), NoiseXform), TEXT("NoiseTransform"));
	Check(Graph->SetGraphParameter<float>(TEXT("Threshold"), static_cast<float>(Threshold)), TEXT("Threshold"));	PCG->Seed = Seed;
	PCG->GenerateLocal(/*bForce=*/ true);


}