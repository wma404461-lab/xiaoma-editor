#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditNamingLibrary.generated.h"

/** 命名规则的脚本入口：返回 JSON，方便 Python 断言和面板展示。 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditNamingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 扫描目录，返回 {"scanned":N,"issues":[{current,suggested,reason,severity,path}]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString ScanPath(const FString& ContentPath);

	/** 扫描并直接改名，返回 {"scanned":N,"renamed":[...],"failed":[...]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString FixPath(const FString& ContentPath);

	/** 单个名字的规则预览，返回 "建议名|原因"。 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString SuggestName(const FString& CurrentName, const FString& ClassName);

	/** 选中 Actor 的标签改成 ENV_{类别}_{变体}_{序号}，返回 {"renamedCount":N,"renamed":[...]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString FixSelectedActorLabels();
};
