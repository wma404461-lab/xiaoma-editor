#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditImportLibrary.generated.h"

/** 贴图导入：按后缀套 sRGB / 法线压缩，导入后按命名规则改名。 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditImportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 把目录下的 png/tga 导入到 DestinationPath。
	 * 返回 {"imported":[...],"renamed":[...],"failed":[...]}
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Import")
	static FString ImportTextures(const FString& SourceDir, const FString& DestinationPath);
};
