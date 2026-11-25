#pragma once

#include "PCGSettings.h"
#include "PCGStreetSurfaceSettings.generated.h"

class UMaterialInterface;

/** Builds road and sidewalk Dynamic Meshes from segment and block points. */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class ENEKOMARTINTOOLS_API UPCGStreetSurfaceSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("StreetSurfaces"); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("ABC", "StreetTitle", "Street Surfaces"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, ClampMin = "0", ForceUnits = "cm"))
	float SidewalkHeight = 15.0f;

	/** Pequeña elevación de la carretera para evitar z-fighting con el suelo. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, ClampMin = "0", ForceUnits = "cm"))
	float RoadHeightOffset = 1.0f;

	/** Invierte la orientación de todas las caras. Actívalo si las mallas se ven del revés. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bFlipFaces = false;

	/** Convierte las mallas al espacio local del actor PCG. Déjalo desactivado salvo que salgan desplazadas. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	bool bOutputInActorSpace = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	TSoftObjectPtr<UMaterialInterface> RoadMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	TSoftObjectPtr<UMaterialInterface> SidewalkMaterial;
};

class FPCGStreetSurfaceElement : public IPCGElement
{
public:
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }

protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};