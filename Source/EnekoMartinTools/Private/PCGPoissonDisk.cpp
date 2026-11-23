#include "PCGPoissonDisk.h"

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGSpatialData.h"
#include "Helpers/PCGHelpers.h"

#define LOCTEXT_NAMESPACE "PCGPoissonDiskElement"

namespace PoissonDiskPrivate
{
	const FName ExclusionsLabel = TEXT("Exclusions");
	constexpr int64 MaxGridCells = 16 * 1024 * 1024;
}

TArray<FPCGPinProperties> UPCGPoissonDiskSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(PCGPinConstants::DefaultInputLabel, EPCGDataType::Spatial, false);
	Pins.Emplace(PoissonDiskPrivate::ExclusionsLabel, EPCGDataType::Spatial, true);
	return Pins;
}

TArray<FPCGPinProperties> UPCGPoissonDiskSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return Pins;
}

FPCGElementPtr UPCGPoissonDiskSettings::CreateElement() const
{
	return MakeShared<FPCGPoissonDiskElement>();
}

bool FPCGPoissonDiskElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGPoissonDiskElement::Execute);

	const UPCGPoissonDiskSettings* Settings = Context->GetInputSettings<UPCGPoissonDiskSettings>();
	check(Settings);

	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	const UPCGSpatialData* Spatial = Inputs.IsEmpty() ? nullptr : Cast<UPCGSpatialData>(Inputs[0].Data);
	if (!Spatial)
	{
		return true;
	}

	const FBox Bounds = Spatial->GetBounds();
	if (!Bounds.IsValid || Settings->MinDistance <= 0.0f)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("BadInput", "Invalid input bounds or MinDistance <= 0."));
		return true;
	}

	TArray<FBox> Exclusions;
	for (const FPCGTaggedData& Tagged : Context->InputData.GetInputsByPin(PoissonDiskPrivate::ExclusionsLabel))
	{
		if (const UPCGBasePointData* ExclusionPoints = Cast<UPCGBasePointData>(Tagged.Data))
		{
			const TConstPCGValueRange<FTransform> Transforms = ExclusionPoints->GetConstTransformValueRange();
			const TConstPCGValueRange<FVector> BoundsMin = ExclusionPoints->GetConstBoundsMinValueRange();
			const TConstPCGValueRange<FVector> BoundsMax = ExclusionPoints->GetConstBoundsMaxValueRange();
			for (int32 Index = 0; Index < ExclusionPoints->GetNumPoints(); ++Index)
			{
				Exclusions.Add(FBox(BoundsMin[Index], BoundsMax[Index]).TransformBy(Transforms[Index]));
			}
		}
		else if (const UPCGSpatialData* ExclusionSpatial = Cast<UPCGSpatialData>(Tagged.Data))
		{
			Exclusions.Add(ExclusionSpatial->GetBounds());
		}
	}

	const double Radius = static_cast<double>(Settings->MinDistance) * 100.0; 
	const double RadiusSq = Radius * Radius;
	const double CellSize = Radius / FMath::Sqrt(2.0);
	const FVector2D Origin(Bounds.Min.X, Bounds.Min.Y);
	const int32 Width = FMath::Max(1, FMath::CeilToInt((Bounds.Max.X - Bounds.Min.X) / CellSize));
	const int32 Height = FMath::Max(1, FMath::CeilToInt((Bounds.Max.Y - Bounds.Min.Y) / CellSize));

	if (static_cast<int64>(Width) * Height > PoissonDiskPrivate::MaxGridCells)
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("TooBig", "MinDistance is too small for these bounds: grid too large."));
		return true;
	}

	TArray<int32> Grid;
	Grid.Init(INDEX_NONE, Width * Height);

	TArray<FVector2D> Samples;
	TArray<int32> Active; 

	auto CellX = [&](double X) { return FMath::Clamp(FMath::FloorToInt((X - Origin.X) / CellSize), 0, Width - 1); };
	auto CellY = [&](double Y) { return FMath::Clamp(FMath::FloorToInt((Y - Origin.Y) / CellSize), 0, Height - 1); };

	auto IsFree = [&](const FVector2D& Q) -> bool
		{
			if (Q.X < Bounds.Min.X || Q.X > Bounds.Max.X || Q.Y < Bounds.Min.Y || Q.Y > Bounds.Max.Y)
			{
				return false;
			}
			for (const FBox& Box : Exclusions)
			{
				if (Q.X >= Box.Min.X && Q.X <= Box.Max.X && Q.Y >= Box.Min.Y && Q.Y <= Box.Max.Y)
				{
					return false;
				}
			}
			const int32 CX = CellX(Q.X), CY = CellY(Q.Y);
			for (int32 Y = FMath::Max(0, CY - 2); Y <= FMath::Min(Height - 1, CY + 2); ++Y)
			{
				for (int32 X = FMath::Max(0, CX - 2); X <= FMath::Min(Width - 1, CX + 2); ++X)
				{
					const int32 Other = Grid[Y * Width + X];
					if (Other != INDEX_NONE && FVector2D::DistSquared(Samples[Other], Q) < RadiusSq)
					{
						return false;
					}
				}
			}
			return true;
		};

	auto Place = [&](const FVector2D& P)
		{
			const int32 NewIndex = Samples.Add(P);
			Active.Add(NewIndex);
			Grid[CellY(P.Y) * Width + CellX(P.X)] = NewIndex;
		};

	FRandomStream Random(Context->GetSeed()); 

	for (int32 Try = 0; Try < 100 && Samples.IsEmpty(); ++Try)
	{
		const FVector2D First(Random.FRandRange(Bounds.Min.X, Bounds.Max.X), Random.FRandRange(Bounds.Min.Y, Bounds.Max.Y));
		if (IsFree(First))
		{
			Place(First);
		}
	}

	const int32 MaxAttempts = FMath::Max(1, Settings->MaxAttempts);
	while (!Active.IsEmpty())
	{
		const int32 ActiveSlot = Random.RandRange(0, Active.Num() - 1);
		const FVector2D Center = Samples[Active[ActiveSlot]];

		bool bFound = false;
		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			const double Angle = Random.FRandRange(0.0, UE_DOUBLE_TWO_PI);
			const double Distance = Random.FRandRange(Radius, 2.0 * Radius); 
			const FVector2D Candidate = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
			if (IsFree(Candidate))
			{
				Place(Candidate);
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			Active.RemoveAtSwap(ActiveSlot); 
		}
	}

	UPCGBasePointData* Points = FPCGContext::NewPointData_AnyThread(Context);
	Points->SetNumPoints(Samples.Num());
	Points->SetExtents(FVector(50.0));
	Points->AllocateProperties(EPCGPointNativeProperties::Transform | EPCGPointNativeProperties::Seed);

	TPCGValueRange<FTransform> Transforms = Points->GetTransformValueRange(/*bAllocate=*/false);
	TPCGValueRange<int32> Seeds = Points->GetSeedValueRange(/*bAllocate=*/false);

	const double Z = Bounds.GetCenter().Z;
	for (int32 Index = 0; Index < Samples.Num(); ++Index)
	{
		const FVector Location(Samples[Index].X, Samples[Index].Y, Z);
		Transforms[Index] = FTransform(Location);
		Seeds[Index] = PCGHelpers::ComputeSeedFromPosition(Location);
	}

	FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
	Output.Data = Points;
	Output.Pin = PCGPinConstants::DefaultOutputLabel;
	return true;
}

#undef LOCTEXT_NAMESPACE