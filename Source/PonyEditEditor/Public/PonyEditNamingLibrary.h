#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditNamingLibrary.generated.h"

/** Script entry for naming rules: return JSON, Convenient Python Assertions and panel display. */
UCLASS()
class PONYEDITEDITOR_API UPonyEditNamingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Scan directory and return {"scanned":N,"issues":[{current,suggested,reason,severity,path}]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString ScanPath(const FString& ContentPath);

	/** Scan and directly rename, return {"scanned":N,"renamed":[...],"failed":[...]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString FixPath(const FString& ContentPath);

	/** Preview of single name rule, return "Suggested name|Reason". */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString SuggestName(const FString& CurrentName, const FString& ClassName);

	/** Select Actor tag changed to ENV_{Category}_{Variant}_{Sequence number}, Back {"renamedCount":N,"renamed":[...]} */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Naming")
	static FString FixSelectedActorLabels();
};
