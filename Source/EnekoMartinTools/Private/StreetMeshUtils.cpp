#include "StreetMeshUtils.h"

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"

using namespace UE::Geometry;

void StreetMeshUtils::InitMesh(FDynamicMesh3& Mesh)
{
	Mesh.EnableAttributes();
	Mesh.Attributes()->EnableMaterialID();
}

void StreetMeshUtils::FinishMesh(FDynamicMesh3& Mesh)
{
	FMeshNormals::InitializeOverlayToPerVertexNormals(Mesh.Attributes()->PrimaryNormals(), false);
	FMeshNormals::QuickRecomputeOverlayNormals(Mesh);
}

void StreetMeshUtils::AppendQuad(FDynamicMesh3& Mesh, const FVector& C00, const FVector& C01,
	const FVector& C10, const FVector& C11, int32 MaterialSlot, bool bWallUV)
{
	FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();
	const FVector Corners[4] = { C00, C01, C10, C11 };

	FVector2f UVCoords[4];
	if (bWallUV)
	{
		const float LengthU = static_cast<float>((C01 - C00).Size() / 100.0);
		const float LengthV = static_cast<float>((C10 - C00).Size() / 100.0);
		UVCoords[0] = FVector2f(0.f, 0.f);
		UVCoords[1] = FVector2f(LengthU, 0.f);
		UVCoords[2] = FVector2f(0.f, LengthV);
		UVCoords[3] = FVector2f(LengthU, LengthV);
	}
	else
	{
		for (int32 Index = 0; Index < 4; ++Index)
		{
			UVCoords[Index] = FVector2f(FVector2D(Corners[Index]) / 100.0);
		}
	}

	int32 Vertices[4], UVElements[4];
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Vertices[Index] = Mesh.AppendVertex(Corners[Index]);
		UVElements[Index] = UVs->AppendElement(UVCoords[Index]);
	}

	for (const FIndex3i& T : { FIndex3i(0, 1, 3), FIndex3i(0, 3, 2) })
	{
		const int32 Triangle = Mesh.AppendTriangle(Vertices[T.A], Vertices[T.B], Vertices[T.C]);
		if (Triangle >= 0)
		{
			UVs->SetTriangle(Triangle, FIndex3i(UVElements[T.A], UVElements[T.B], UVElements[T.C]));
			Mesh.Attributes()->GetMaterialID()->SetValue(Triangle, MaterialSlot);
		}
	}
}