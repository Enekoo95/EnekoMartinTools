#include "PerlinNoiseGenerator.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace
{
	const FName TagCube(TEXT("PerlinCube"));
	const FName TagISM(TEXT("PerlinCubesISM"));
}

float UPerlinNoiseGenerator::GetPerlinValue(int32 X, int32 Y, float Scale, int32 Seed)
{
	const FVector2D P(X * Scale + Seed * 31.7f, Y * Scale + Seed * 17.3f);
	return FMath::Clamp((FMath::PerlinNoise2D(P) + 1.f) * 0.5f, 0.f, 1.f);
}

UTexture2D* UPerlinNoiseGenerator::GeneratePerlinTexture(int32 Width, int32 Height, float Scale, int32 Seed)
{
	Width = FMath::Clamp(Width, 8, 4096);
	Height = FMath::Clamp(Height, 8, 4096);

	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Tex) { return nullptr; }

	Tex->SRGB = false;
	Tex->Filter = TF_Bilinear;
	Tex->AddressX = TA_Wrap;
	Tex->AddressY = TA_Wrap;

	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const uint8 V = (uint8)FMath::RoundToInt(GetPerlinValue(X, Y, Scale, Seed) * 255.f);
			Pixels[Y * Width + X] = FColor(V, V, V, 255);
		}
	}
	Mip.BulkData.Unlock();

	Tex->UpdateResource();
	return Tex;
}

int32 UPerlinNoiseGenerator::ClearPerlinCubes()
{
#if WITH_EDITOR
	if (!GEditor) { return 0; }
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) { return 0; }

	TArray<AActor*> ToDelete;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TagCube) || It->ActorHasTag(TagISM))
		{
			ToDelete.Add(*It);
		}
	}
	for (AActor* Actor : ToDelete)
	{
		World->EditorDestroyActor(Actor, true);
	}
	return ToDelete.Num();
#else
	return 0;
#endif
}

int32 UPerlinNoiseGenerator::SpawnPerlinCubes(UStaticMesh* CubeMesh, int32 Width, int32 Height,
	float Scale, int32 Seed, float Threshold, float Spacing)
{
#if WITH_EDITOR
	if (!GEditor || !CubeMesh) { return 0; }

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) { return 0; }

	Width = FMath::Clamp(Width, 1, 512);
	Height = FMath::Clamp(Height, 1, 512);
	Spacing = FMath::Max(Spacing, 1.f);

	ClearPerlinCubes();

	int32 NumCubes = 0;
	const FVector CubeScale(Spacing / 100.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (GetPerlinValue(X, Y, Scale, Seed) > Threshold)
			{
				const FVector Location(X * Spacing, Y * Spacing, 0.f);

				AStaticMeshActor* Cube = World->SpawnActor<AStaticMeshActor>(
					Location, FRotator::ZeroRotator, Params);
				if (Cube)
				{
					Cube->SetMobility(EComponentMobility::Movable);
					Cube->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
					Cube->SetActorScale3D(CubeScale);
					Cube->Tags.Add(TagCube);
					++NumCubes;
				}
			}
		}
	}
	return NumCubes;
#else
	return 0;
#endif
}

int32 UPerlinNoiseGenerator::SpawnPerlinCubesISM(UStaticMesh* CubeMesh, int32 Width, int32 Height,
	float Scale, int32 Seed, float Threshold, float Spacing)
{
#if WITH_EDITOR
	if (!GEditor || !CubeMesh) { return 0; }

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) { return 0; }

	Width = FMath::Clamp(Width, 1, 1024);
	Height = FMath::Clamp(Height, 1, 1024);
	Spacing = FMath::Max(Spacing, 1.f);

	ClearPerlinCubes();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Holder = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Holder) { return 0; }

	UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(
		Holder, TEXT("PerlinISM"), RF_Transactional);
	ISM->SetStaticMesh(CubeMesh);
	ISM->SetMobility(EComponentMobility::Movable);
	Holder->SetRootComponent(ISM);
	Holder->AddInstanceComponent(ISM);
	ISM->RegisterComponent();

	const FVector CubeScale(Spacing / 100.f);
	int32 NumCubes = 0;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			if (GetPerlinValue(X, Y, Scale, Seed) > Threshold)
			{
				const FTransform T(FRotator::ZeroRotator, FVector(X * Spacing, Y * Spacing, 0.f), CubeScale);
				ISM->AddInstance(T);
				++NumCubes;
			}
		}
	}

	Holder->Tags.Add(TagISM);
	Holder->SetActorLabel(TEXT("PerlinCubesISM"));
	return NumCubes;
#else
	return 0;
#endif
}