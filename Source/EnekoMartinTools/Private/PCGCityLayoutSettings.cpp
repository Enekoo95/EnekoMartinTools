#include "PCGCityLayoutSettings.h"

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGBasePointData.h"
#include "Helpers/PCGHelpers.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "PCGCityLayout"

namespace CityLayoutPrivate
{
	const FName BlocksLabel = TEXT("Blocks");
	const FName SegmentsLabel = TEXT("Segments");
	const FName LotsLabel = TEXT("Lots");
	const FName ParksLabel = TEXT("Parks");

	struct FPointSpec
	{
		FTransform Transform;
		FVector BoundsMin;
		FVector BoundsMax;
	};

	void EmitPoints(FPCGContext* Context, FName Pin, const TArray<FPointSpec>& Specs)
	{
		UPCGBasePointData* Points = FPCGContext::NewPointData_AnyThread(Context);
		Points->SetNumPoints(Specs.Num());
		Points->AllocateProperties(EPCGPointNativeProperties::Transform | EPCGPointNativeProperties::BoundsMin
			| EPCGPointNativeProperties::BoundsMax | EPCGPointNativeProperties::Seed);

		TPCGValueRange<FTransform> Transforms = Points->GetTransformValueRange(/*bAllocate=*/false);
		TPCGValueRange<FVector> BoundsMin = Points->GetBoundsMinValueRange(/*bAllocate=*/false);
		TPCGValueRange<FVector> BoundsMax = Points->GetBoundsMaxValueRange(/*bAllocate=*/false);
		TPCGValueRange<int32> Seeds = Points->GetSeedValueRange(/*bAllocate=*/false);

		for (int32 Index = 0; Index < Specs.Num(); ++Index)
		{
			Transforms[Index] = Specs[Index].Transform;
			BoundsMin[Index] = Specs[Index].BoundsMin;
			BoundsMax[Index] = Specs[Index].BoundsMax;
			Seeds[Index] = PCGHelpers::ComputeSeedFromPosition(Specs[Index].Transform.GetLocation());
		}

		FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
		Output.Data = Points;
		Output.Pin = Pin;
	}

	/** Rectangulo plano: bounds en XY, grosor cero en Z (lo que espera Street Surfaces). */
	FPointSpec FlatRect(const FTransform& ActorXform, const FVector2D& Center, const FVector2D& Size)
	{
		FPointSpec S;
		S.Transform = FTransform(FRotator::ZeroRotator, FVector(Center.X, Center.Y, 0.0)) * ActorXform;
		S.BoundsMin = FVector(-Size.X * 0.5, -Size.Y * 0.5, 0.0);
		S.BoundsMax = FVector(Size.X * 0.5, Size.Y * 0.5, 0.0);
		return S;
	}
}

TArray<FPCGPinProperties> UPCGCityLayoutSettings::InputPinProperties() const
{
	return {};
}

TArray<FPCGPinProperties> UPCGCityLayoutSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(CityLayoutPrivate::BlocksLabel, EPCGDataType::Point);
	Pins.Emplace(CityLayoutPrivate::SegmentsLabel, EPCGDataType::Point);
	Pins.Emplace(CityLayoutPrivate::LotsLabel, EPCGDataType::Point);
	Pins.Emplace(CityLayoutPrivate::ParksLabel, EPCGDataType::Point);
	return Pins;
}

FPCGElementPtr UPCGCityLayoutSettings::CreateElement() const
{
	return MakeShared<FPCGCityLayoutElement>();
}

bool FPCGCityLayoutElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGCityLayoutElement::Execute);
	using namespace CityLayoutPrivate;

	const UPCGCityLayoutSettings* Settings = Context->GetInputSettings<UPCGCityLayoutSettings>();
	check(Settings);

	AActor* Target = Context->GetTargetActor(nullptr);
	FCityLayoutParams Params = Settings->Fallback;
	FTransform ActorXform = FTransform::Identity;
	if (Target)
	{
		ActorXform = Target->GetActorTransform();
		ActorXform.SetScale3D(FVector::OneVector);
		if (const ICityLayoutSource* Source = Cast<ICityLayoutSource>(Target))
		{
			Params = Source->GetCityLayoutParams();
		}
	}

	const FCityLayout Layout = FCityLayoutBuilder::Build(Params);

	TArray<FPointSpec> Blocks, Parks, Segments, Lots;

	for (const FCityBlock& Block : Layout.Blocks)
	{
		(Block.bPark ? Parks : Blocks).Add(FlatRect(ActorXform, Block.Center, Block.Size));
	}
	for (const FCityRect& Road : Layout.Roads)
	{
		Segments.Add(FlatRect(ActorXform, Road.Center, Road.Size));
	}
	for (const FCityLot& Lot : Layout.Lots)
	{
		FPointSpec S;
		S.Transform = FTransform(FRotator(0.f, Lot.Yaw, 0.f), Lot.Location) * ActorXform;
		S.BoundsMin = FVector(-Lot.Depth * 0.5, -Lot.Width * 0.5, 0.0);
		S.BoundsMax = FVector(Lot.Depth * 0.5, Lot.Width * 0.5, Lot.Height);
		Lots.Add(S);
	}

	EmitPoints(Context, BlocksLabel, Blocks);
	EmitPoints(Context, SegmentsLabel, Segments);
	EmitPoints(Context, LotsLabel, Lots);
	EmitPoints(Context, ParksLabel, Parks);

	UE_LOG(LogTemp, Verbose, TEXT("CityLayout: %d blocks, %d parks, %d segments, %d lots"),
		Blocks.Num(), Parks.Num(), Segments.Num(), Lots.Num());
	return true;
}

#undef LOCTEXT_NAMESPACE
