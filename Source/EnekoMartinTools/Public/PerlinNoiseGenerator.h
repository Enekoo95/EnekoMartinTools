// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PerlinNoiseGenerator.generated.h"

/**
 * 
 */
UCLASS()
class ENEKOMARTINTOOLS_API UPerlinNoiseGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "ABCTools|Perlin")
	static float GetPerlinValue(int32 X, int32 Y, float Scale, int32 Seed);
	UFUNCTION(BlueprintCallable, Category = "ABCTools|Perlin")
	static UTexture2D* GeneratePerlinTexture(int32 Width = 256, int32 Height = 256,
		float Scale = 0.05f, int32 Seed = 0);
};
