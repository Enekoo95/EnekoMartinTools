#pragma once

#include "DynamicMesh/DynamicMesh3.h"

namespace StreetMeshUtils
{
	void InitMesh(UE::Geometry::FDynamicMesh3& Mesh);

	void AppendQuad(UE::Geometry::FDynamicMesh3& Mesh,
		const FVector& C00, const FVector& C01,
		const FVector& C10, const FVector& C11,
		int32 MaterialSlot = 0, bool bWallUV = false);

	void FinishMesh(UE::Geometry::FDynamicMesh3& Mesh);
}