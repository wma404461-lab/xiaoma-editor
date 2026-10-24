#pragma once

#include "CoreMinimal.h"

/**
 * Mesh Organization: Instance-level Fit + Asset-level pivot alignment.
 * M0 Only do「Align to base」, Bake scale / Collision rebuild at M2 Fill.
 */
class FPonyMeshOps
{
public:
	/** Group into bounding box Actor Move entire object down so its bounding box lowest point lands at Z=0. */
	static int32 SnapSelectedActorsToGround(FString& OutMessage);

	/** Move selected Actor Execute asset-level pivot origin StaticMesh Move asset pivot to「XY Center + Bottom face」, And sync simple collision. */
	static int32 PivotMeshesToBottom(FString& OutMessage);
};
