#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditMaterialLibrary.generated.h"

/**
 * M2 材质架：8 个标准母材质的参数契约、MI 生成、契约校验和材质架查询。
 *
 * 所有入口返回 JSON，方便面板展示和 Python 自动化断言。
 * 母材质路径固定 /Game/PonyEdit/Materials/Masters，MI 默认放 /Game/PonyEdit/Materials/Instances。
 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditMaterialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 材质架：8 个母材质是否存在、契约是否齐全、各有多少 MI。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString GetRack();

	/** 检查单个母材质的参数契约，返回缺失/多余参数。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString ValidateMaster(const FString& MasterPath);

	/**
	 * 按母材质契约创建（或复用）MI，套上契约默认参数并保存。
	 * 名字不是 MI_ 开头时自动补前缀。
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString CreateInstance(const FString& MasterPath, const FString& PackagePath, const FString& AssetName);

	/** 用选中 Actor 的网格名建 MI（MI_<Mesh>_<类型>），并直接套到选中 Actor。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString CreateInstanceForSelected(const FString& MasterPath, const FString& PackagePath);

	/** 把材质套到选中的 Actor；SlotIndex < 0 表示所有槽位。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString ApplyToSelected(const FString& MaterialPath, int32 SlotIndex);

	/** 读 MI 的父级、契约参数当前值，供面板/自动化验证。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString GetInstanceInfo(const FString& MaterialPath);
};
