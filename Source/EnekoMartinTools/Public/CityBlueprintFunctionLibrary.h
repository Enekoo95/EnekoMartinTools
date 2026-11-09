#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CityBlueprintFunctionLibrary.generated.h"

class UPCGComponent;

UCLASS()
class ENEKOMARTINTOOLS_API UCityBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "City|PCG")
	static void ApplyPCGParameters(
		FVector GridExtent = FVector(2000,2000,10),
		FVector CellSize = FVector(200,200, 10),
		double NoiseSize = 10.0,
		double Threshold = 0.5,
		int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category = "City|PCG")
	static UPCGComponent* FindPCGComponent();
};