#include "PCGStreetSurfaceSettings.h"

#include "StreetMeshUtils.h"

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGBasePointData.h"
#include "Data/PCGDynamicMeshData.h"
#include "DynamicMesh/MeshTransforms.h"
#include "GameFramework/Actor.h"
#include "Helpers/PCGHelpers.h"
#include "Materials/MaterialInterface.h"

#define LOCTEXT_NAMESPACE "PCGStreetSurfaces"

using namespace UE::Geometry;

namespace StreetSurfacesPrivate
{
	const FName SegmentsLabel = TEXT("Segments");
	const FName BlocksLabel = TEXT("Blocks");
	const FName RoadsLabel = TEXT("Roads");
	const FName SidewalksLabel = TEXT("Sidewalks");

	void ForEachPointBox(FPCGContext* Context, FName Pin,
		TFunctionRef<void(const FTransform&, const FVector&, const FVector&)> Fn)
	{
		for (const FPCGTaggedData& Tagged : Context->InputData.GetInputsByPin(Pin))
		{
			const UPCGBasePointData* Points = Cast<UPCGBasePointData>(Tagged.Data);
			if (!Points)
			{
				continue;
			}

			const TConstPCGValueRange<FTransform> Transforms = Points->GetConstTransformValueRange();
			const TConstPCGValueRange<FVector> BoundsMin = Points->GetConstBoundsMinValueRange();
			const TConstPCGValueRange<FVector> BoundsMax = Points->GetConstBoundsMaxValueRange();
			for (int32 Index = 0; Index < Points->GetNumPoints(); ++Index)
			{
				Fn(Transforms[Index], BoundsMin[Index], BoundsMax[Index]);
			}
		}
	}

	FTransform GetTargetTransform(FPCGContext* Context)
	{
		if (const AActor* Target = Context->GetTargetActor(nullptr))
		{
			return Target->GetActorTransform();
		}
		return FTransform::Identity;
	}

	void Emit(FPCGContext* Context, FDynamicMesh3&& Mesh, UMaterialInterface* Material, FName Pin)
	{
		UPCGDynamicMeshData* Data = FPCGContext::NewObject_AnyThread<UPCGDynamicMeshData>(Context);
		Data->Initialize(MoveTemp(Mesh), { Material });

		FPCGTaggedData& Output = Context->OutputData.TaggedData.Emplace_GetRef();
		Output.Data = Data;
		Output.Pin = Pin;
	}
}

TArray<FPCGPinProperties> UPCGStreetSurfaceSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(StreetSurfacesPrivate::SegmentsLabel, EPCGDataType::Point);
	Pins.Emplace(StreetSurfacesPrivate::BlocksLabel, EPCGDataType::Point);
	return Pins;
}

TArray<FPCGPinProperties> UPCGStreetSurfaceSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins;
	Pins.Emplace(StreetSurfacesPrivate::RoadsLabel, EPCGDataType::DynamicMesh);
	Pins.Emplace(StreetSurfacesPrivate::SidewalksLabel, EPCGDataType::DynamicMesh);
	return Pins;
}

FPCGElementPtr UPCGStreetSurfaceSettings::CreateElement() const
{
	return MakeShared<FPCGStreetSurfaceElement>();
}

bool FPCGStreetSurfaceElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGStreetSurfaceElement::Execute);
	using namespace StreetSurfacesPrivate;

	const UPCGStreetSurfaceSettings* Settings = Context->GetInputSettings<UPCGStreetSurfaceSettings>();
	check(Settings);

	UMaterialInterface* RoadMaterial = Settings->RoadMaterial.LoadSynchronous();
	UMaterialInterface* SidewalkMaterial = Settings->SidewalkMaterial.LoadSynchronous();

	const FVector RoadLift(0.0, 0.0, Settings->RoadHeightOffset);
	const FVector SidewalkUp(0.0, 0.0, Settings->SidewalkHeight);

	// ---------------------------------------------------------------- Carretera
	FDynamicMesh3 RoadMesh;
	StreetMeshUtils::InitMesh(RoadMesh);

	ForEachPointBox(Context, SegmentsLabel,
		[&RoadMesh, &RoadLift](const FTransform& T, const FVector& Min, const FVector& Max)
		{
			// C00=(minX,minY) C01=(maxX,minY) C10=(minX,maxY) C11=(maxX,maxY) -> normal +Z
			StreetMeshUtils::AppendQuad(RoadMesh,
				T.TransformPosition(FVector(Min.X, Min.Y, 0.0)) + RoadLift,
				T.TransformPosition(FVector(Max.X, Min.Y, 0.0)) + RoadLift,
				T.TransformPosition(FVector(Min.X, Max.Y, 0.0)) + RoadLift,
				T.TransformPosition(FVector(Max.X, Max.Y, 0.0)) + RoadLift,
				/*MaterialSlot=*/0);
		});

	// ------------------------------------------------------------------- Acera
	FDynamicMesh3 SidewalkMesh;
	StreetMeshUtils::InitMesh(SidewalkMesh);

	ForEachPointBox(Context, BlocksLabel,
		[&SidewalkMesh, &SidewalkUp](const FTransform& T, const FVector& Min, const FVector& Max)
		{
			// Esquinas en sentido antihorario visto desde +Z.
			const FVector P[4] =
			{
				T.TransformPosition(FVector(Min.X, Min.Y, 0.0)),
				T.TransformPosition(FVector(Max.X, Min.Y, 0.0)),
				T.TransformPosition(FVector(Max.X, Max.Y, 0.0)),
				T.TransformPosition(FVector(Min.X, Max.Y, 0.0)),
			};

			// Tapa superior (normal +Z)
			StreetMeshUtils::AppendQuad(SidewalkMesh,
				P[0] + SidewalkUp, P[1] + SidewalkUp,
				P[3] + SidewalkUp, P[2] + SidewalkUp, 0);

			// Paredes (normal hacia fuera): C00=A C01=B C10=A+Up C11=B+Up
			for (int32 Edge = 0; Edge < 4; ++Edge)
			{
				const FVector& A = P[Edge];
				const FVector& B = P[(Edge + 1) % 4];
				StreetMeshUtils::AppendQuad(SidewalkMesh, A, B, A + SidewalkUp, B + SidewalkUp, 0, /*bWallUV=*/true);
			}
		});

	// Opcional: pasar de espacio mundo a espacio local del actor PCG.
	if (Settings->bOutputInActorSpace)
	{
		const FTransformSRT3d ActorTransform(GetTargetTransform(Context));
		MeshTransforms::ApplyTransformInverse(RoadMesh, ActorTransform, true);
		MeshTransforms::ApplyTransformInverse(SidewalkMesh, ActorTransform, true);
	}

	if (Settings->bFlipFaces)
	{
		RoadMesh.ReverseOrientation(false);
		SidewalkMesh.ReverseOrientation(false);
	}

	UE_LOG(LogTemp, Verbose, TEXT("StreetSurfaces: Roads tris=%d center=%s | Sidewalks tris=%d center=%s"),
		RoadMesh.TriangleCount(), *RoadMesh.GetBounds().Center().ToString(),
		SidewalkMesh.TriangleCount(), *SidewalkMesh.GetBounds().Center().ToString());

	StreetMeshUtils::FinishMesh(RoadMesh);
	StreetMeshUtils::FinishMesh(SidewalkMesh);

	Emit(Context, MoveTemp(RoadMesh), RoadMaterial, RoadsLabel);
	Emit(Context, MoveTemp(SidewalkMesh), SidewalkMaterial, SidewalksLabel);
	return true;
}

#undef LOCTEXT_NAMESPACE