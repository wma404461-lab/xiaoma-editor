#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditTestLibrary.generated.h"

class UStaticMesh;
class UMaterialInterface;

/** for automation scripts/verification-only probes, not involved in editor logic. */
UCLASS()
class PONYEDITEDITOR_API UPonyEditTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** directly read source mesh (all LOD vertex) bounding box, bypassing potentially delayed render bounding box. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FBox GetMeshSourceBounds(UStaticMesh* Mesh);

	/** read-only texture settings, avoid Python reflection guess property name. Format: srgb=0;compression=1;lodgroup=0 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FString GetTextureSettings(class UTexture2D* Texture);

	/** material compile status: exists=1;compilingOrError=0;hasResource=1 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FString GetMaterialCompileStatus(UMaterialInterface* Material);
};
