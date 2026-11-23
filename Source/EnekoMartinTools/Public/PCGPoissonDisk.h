// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PCGSettings.h"
#include "PCGPoissonDisk.generated.h"

UCLASS(BlueprintType, ClassGroup = (Procedural))
class ENEKOMARTINTOOLS_API UPCGPoissonDiskSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("PCGPoissonDisk")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGPoissonDisk", "NodeTitle", "Poisson Disk Sampler"); }
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGPoissonDisk", "NodeTooltip", "Scatters points at random with a minimum distance between them."); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Sampler; }
#endif
	virtual bool UseSeed() const override { return true; }

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PCG|PoissonDisk", meta = (PCG_Overridable, ClampMin = "0.1", ForceUnits = "m"))
	float MinDistance = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PCG|PoissonDisk", meta = (PCG_Overridable, ClampMin = "1"))
	int32 MaxAttempts = 30;
};

class FPCGPoissonDiskElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};