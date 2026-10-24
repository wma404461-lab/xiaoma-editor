#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditScatterLibrary.generated.h"

/**
 * M3 Planting machine v1: When selected Actor cast a ray downward within the bounding box, scatter a StaticMeshActor.
 *
 * parameter walk JSON, Back JSON; supports slope filtering, clustering, random size/direction, ground adherence/bottom landing, fixed random seed.
 * This version is「button to scatter」, viewport drag brush and HISM batching in next version.
 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditScatterLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * scatter vegetation on selected surfaces/Prop.
	 * parameters: meshPath, count, radius, minScale, maxScale, maxSlopeDegrees,
	 *       clusterCount, clusterRadius, surfaceOffset, randomSeed, alignToNormal,
	 *       dropToBase; available when no selection center:[x,y,z] + traceHeight.
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString ScatterOnSelection(const FString& ParamsJson);

	/** delete all currently in the level PonyScatter tagged Actor. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString ClearScatter();

	/** read-only statistics: how many in current level PonyScatter Actor. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Scatter")
	static FString GetScatterStats();
};
