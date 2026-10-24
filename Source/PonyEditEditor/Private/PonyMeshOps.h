#pragma once

#include "CoreMinimal.h"

/**
 * 网格整理：实例级贴合 + 资产级枢轴归底。
 * M0 只做「归底」，烘焙缩放 / 碰撞重建在 M2 补。
 */
class FPonyMeshOps
{
public:
	/** 把选中的 Actor 整体下移，使包围盒最低点落在 Z=0。 */
	static int32 SnapSelectedActorsToGround(FString& OutMessage);

	/** 把选中 Actor 用到的 StaticMesh 资产枢轴移到「XY 中心 + 底面」，并同步简单碰撞。 */
	static int32 PivotMeshesToBottom(FString& OutMessage);
};
