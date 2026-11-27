#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CityLayout.generated.h"

USTRUCT(BlueprintType)
struct ENEKOMARTINTOOLS_API FCityLayoutParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksX = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksY = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200"))
	float StreetWidth = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200"))
	float MinBlockSize = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200"))
	float MaxBlockSize = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "200"))
	float TargetLotSize = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MinFootprint = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MaxFootprint = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "100"))
	float MinBuildingHeight = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "100"))
	float MaxBuildingHeight = 2000.f;

	/** Altura de la acera: los edificios se colocan encima. Debe coincidir con SidewalkHeight del nodo Street Surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0"))
	float SurfaceHeight = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin")
	bool bUsePerlinNoise = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin", meta = (ClampMin = "0.01", ClampMax = "2"))
	float BlockSizeNoiseScale = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ParkNoiseScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkThreshold = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HeightNoiseScale = 0.2f;

	/** Probabilidad de parque cuando no se usa Perlin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	int32 Seed = 12345;
};

/** Rectangulo en el plano XY, en espacio local del actor (centrado en el origen). */
struct FCityRect
{
	FVector2D Center = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
};

struct FCityBlock : FCityRect
{
	int32 X = 0;
	int32 Y = 0;
	bool bPark = false;
};

/** Parcela edificable. Location esta en espacio local y ya incluye SurfaceHeight. */
struct FCityLot
{
	FVector Location = FVector::ZeroVector;
	float Yaw = 0.f;
	float Depth = 0.f;   // eje X del edificio
	float Width = 0.f;   // eje Y del edificio
	float Height = 0.f;
	int32 BlockX = 0;
	int32 BlockY = 0;
};

struct FCityLayout
{
	FVector2D TotalSize = FVector2D::ZeroVector;
	TArray<FCityBlock> Blocks;   // todas las manzanas (parques incluidos)
	TArray<FCityRect> Roads;     // tiras de calle sin solapes (los cruces los cubre la calle vertical)
	TArray<FCityLot> Lots;       // edificios (solo manzanas que no son parque)
};

class ENEKOMARTINTOOLS_API FCityLayoutBuilder
{
public:
	/** Determinista: mismos parametros -> mismo resultado. Sin dependencias de editor ni del mundo. */
	static FCityLayout Build(const FCityLayoutParams& Params);
};

/** Lo implementa el actor que "posee" la ciudad (p.ej. ACityScene). El nodo PCG City Layout lee de aqui, asi actor y grafo siempre coinciden. */
UINTERFACE(MinimalAPI)
class UCityLayoutSource : public UInterface
{
	GENERATED_BODY()
};

class ENEKOMARTINTOOLS_API ICityLayoutSource
{
	GENERATED_BODY()

public:
	virtual FCityLayoutParams GetCityLayoutParams() const = 0;
};
