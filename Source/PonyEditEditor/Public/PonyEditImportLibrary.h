#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditImportLibrary.generated.h"

/** texture import: match by suffix sRGB / Normal compression, rename after import according to naming rules. */
UCLASS()
class PONYEDITEDITOR_API UPonyEditImportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Scan directory under png/tga Import to DestinationPath.
	 * Back {"imported":[...],"renamed":[...],"failed":[...]}
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Import")
	static FString ImportTextures(const FString& SourceDir, const FString& DestinationPath);
};
