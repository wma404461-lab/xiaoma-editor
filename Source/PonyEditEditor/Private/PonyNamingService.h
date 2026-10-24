#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/** One naming issue.Severity: 2=Must be changed, 1=Recommended to change. */
struct FPonyNamingIssue
{
	FAssetData Asset;
	FString CurrentName;
	FString SuggestedName;
	FString Reason;
	int32 Severity = 1;
};

/**
 * M1 Keep: 
 *   SM_ / M_ / MI_ / T_ / BP_ / DA_ / NS_ / SK_ / S_
 *   Prefix after PascalCase; Sequence padded to two digits; remove _final / _copy / (1) / Copy / _v2.
 *   Only rename, no cross-directory asset move.
 */
class FPonyNamingService
{
public:
	static FString GetPrefixForClass(const FString& ClassName);

	/** Scan all assets in a directory and collect issues. */
	static void ScanPath(const FString& ContentPath, TArray<FPonyNamingIssue>& OutIssues, int32& OutScanned);

	/** Scan specified assets. */
	static void ScanAssets(const TArray<FAssetData>& Assets, TArray<FPonyNamingIssue>& OutIssues);

	/** Batch rename according to suggestions; return number of successful operations. */
	static int32 ApplyFixes(const TArray<FPonyNamingIssue>& Issues, TArray<FString>& OutRenamed, TArray<FString>& OutFailed);

	/** Rule core: given a name and class, calculate suggested name and reason. Empty reason = Compliance. */
	static FString SuggestName(const FString& CurrentName, const FString& ClassName, FString& OutReason);

	/** Select Actor tag changed to ENV_{Category}_{Variant}_{Sequence number}, Derive from grid name. */
	static int32 FixSelectedActorLabels(TArray<FString>& OutRenamed);

private:
	static bool IsAllDigits(const FString& Token);
};
