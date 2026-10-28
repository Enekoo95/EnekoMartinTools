// Fill out your copyright notice in the Description page of Project Settings.


#include "PerlinNoiseGenerator.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"

float UPerlinNoiseGenerator::GetPerlinValue(int32 X, int32 Y, float Scale, int32 Seed)
{
	// The seed offsets the sample point: same seed → same texture
	const FVector2D P(X * Scale + Seed * 31.7f, Y * Scale + Seed * 17.3f);
	// PerlinNoise2D returns [-1, 1]; remap it to [0, 1]
	return FMath::Clamp((FMath::PerlinNoise2D(P) + 1.f) * 0.5f, 0.f, 1.f);
}

UTexture2D* UPerlinNoiseGenerator::GeneratePerlinTexture(int32 Width, int32 Height, float Scale, int32 Seed)
{
	Width = FMath::Clamp(Width, 8, 4096); Height = FMath::Clamp(Height, 8, 4096);
	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	if (!Tex) { return nullptr; }
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	for (int32 Y = 0; Y < Height; ++Y)
		for (int32 X = 0; X < Width; ++X)
		{
			const uint8 V = (uint8)FMath::RoundToInt(GetPerlinValue(X, Y, Scale, Seed) * 255.f);
			Pixels[Y * Width + X] = FColor(V, V, V, 255);
		}
	Mip.BulkData.Unlock();
	Tex->UpdateResource(); // uploads the pixels to the GPU
	return Tex;
}

int32 UPerlinNoiseGenerator::SpawnPerlinCubes(UStaticMesh* CubeMesh, int32 Width, int32 Height,
	float Scale, int32 Seed, float Threshold, float Spacing)
{

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) { return 0; }

	Width = FMath::Clamp(Width, 1, 4096);
	Height = FMath::Clamp(Height, 1, 4096);
	Spacing = FMath::Max(Spacing, 1.f);

	static const FName CubeTag(TEXT("PerlinCube"));

	TArray<AActor*> ToDelete;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(CubeTag)) { ToDelete.Add(*It); }
	}
	for (AActor* Actor : ToDelete)
	{
		World->EditorDestroyActor(Actor, true);
	}

	int32 NumCubes = 0;
	const FVector CubeScale(Spacing / 100.f);   

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const float V = GetPerlinValue(X, Y, Scale, Seed);
			if (V > Threshold)
			{
				const FVector Location(X * Spacing, Y * Spacing, 0.f);

				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AStaticMeshActor* Cube = World->SpawnActor<AStaticMeshActor>(
					Location, FRotator::ZeroRotator, Params);
				if (Cube)
				{
					Cube->SetMobility(EComponentMobility::Movable);
					Cube->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
					Cube->SetActorScale3D(CubeScale);
					Cube->Tags.Add(CubeTag);   
					++NumCubes;
				}
			}
		}
	}
	return NumCubes;   

	return 0;
}