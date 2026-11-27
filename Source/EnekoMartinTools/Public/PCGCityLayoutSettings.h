#pragma once

#include "PCGSettings.h"
#include "CityLayout.h"
#include "PCGCityLayoutSettings.generated.h"

/**
 * Genera el layout de la ciudad como puntos con bounds:
 *   Blocks   -> manzanas edificables (entrada de Street Surfaces, pin Blocks)
 *   Segments -> calles (entrada de Street Surfaces, pin Segments)
 *   Lots     -> un punto por edificio (bounds = caja del edificio)
 *   Parks    -> manzanas que son parque
 * Si el actor objetivo implementa ICityLayoutSource usa sus parametros; si no, usa Fallback.
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class ENEKOMARTINTOOLS_API UPCGCityLayoutSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("CityLayout"); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGCityLayout", "NodeTitle", "City Layout"); }
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGCityLayout", "NodeTooltip", "Generates blocks, streets, building lots and parks from the owning actor's city parameters."); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;

public:
	/** Se usa solo si el actor objetivo no implementa ICityLayoutSource. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings)
	FCityLayoutParams Fallback;
};

class FPCGCityLayoutElement : public IPCGElement
{
public:
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
	// Los parametros viven en el actor, no en el grafo: sin esto PCG reutilizaria resultados viejos.
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }

protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
