#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditScatterLibrary.generated.h"

/**
 * M3 种草机 v1：在选中 Actor 的包围盒范围内向下打射线，撒一片 StaticMeshActor。
 *
 * 参数走 JSON，返回 JSON；支持坡度过滤、聚簇、随机尺寸/朝向、贴地/落底、固定随机种子。
 * 这一版是「按钮撒一片」，视口拖拽笔刷和 HISM 合批在下一版。
 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditScatterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 在选中表面上撒植被/道具。
	 * 参数：meshPath, count, radius, minScale, maxScale, maxSlopeDegrees,
	 *       clusterCount, clusterRadius, surfaceOffset, randomSeed, alignToNormal,
	 *       dropToBase；无选中时可用 center:[x,y,z] + traceHeight。
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString ScatterOnSelection(const FString& ParamsJson);

	/** 删除当前关卡里所有 PonyScatter 标签的 Actor。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString ClearScatter();

	/** 只读统计：当前关卡有多少 PonyScatter Actor。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString GetScatterStats();
};
