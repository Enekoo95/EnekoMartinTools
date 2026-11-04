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
		int32 Width = 50,
		int32 Height = 50,
		double Spacing = 100.0,
		double NoiseSize = 10.0,
		double Threshold = 0.5,
		int32 Seed = 0);

private:
	static UPCGComponent* FindPCGComponent();
};