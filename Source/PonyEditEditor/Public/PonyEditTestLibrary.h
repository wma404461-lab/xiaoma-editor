#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditTestLibrary.generated.h"

class UStaticMesh;
class UMaterialInterface;

/** 给自动化脚本/验收用的只读探针，不参与编辑器逻辑。 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditTestLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 直接读源网格（所有 LOD 的顶点）的包围盒，绕过可能延迟的渲染包围盒。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FBox GetMeshSourceBounds(UStaticMesh* Mesh);

	/** 只读贴图设置，避免 Python 反射猜属性名。格式：srgb=0;compression=1;lodgroup=0 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FString GetTextureSettings(class UTexture2D* Texture);

	/** 材质编译状态：exists=1;compilingOrError=0;hasResource=1 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Test")
	static FString GetMaterialCompileStatus(UMaterialInterface* Material);
};
