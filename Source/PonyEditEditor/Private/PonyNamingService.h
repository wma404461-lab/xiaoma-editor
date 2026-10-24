#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/** 一条命名问题。Severity：2=必须改，1=建议改。 */
struct FPonyNamingIssue
{
	FAssetData Asset;
	FString CurrentName;
	FString SuggestedName;
	FString Reason;
	int32 Severity = 1;
};

/**
 * M1 命名规则：
 *   SM_ / M_ / MI_ / T_ / BP_ / DA_ / NS_ / SK_ / S_
 *   前缀后用 PascalCase；序号补两位；去掉 _final / _copy / (1) / 副本 / _v2。
 *   只改名字，不跨目录搬资产。
 */
class FPonyNamingService
{
public:
	static FString GetPrefixForClass(const FString& ClassName);

	/** 扫描一个目录下的全部资产，收集问题。 */
	static void ScanPath(const FString& ContentPath, TArray<FPonyNamingIssue>& OutIssues, int32& OutScanned);

	/** 扫描给定资产。 */
	static void ScanAssets(const TArray<FAssetData>& Assets, TArray<FPonyNamingIssue>& OutIssues);

	/** 按建议名批量改名；返回成功条数。 */
	static int32 ApplyFixes(const TArray<FPonyNamingIssue>& Issues, TArray<FString>& OutRenamed, TArray<FString>& OutFailed);

	/** 规则核心：给一个名字和类，算出建议名与原因。空原因 = 合规。 */
	static FString SuggestName(const FString& CurrentName, const FString& ClassName, FString& OutReason);

	/** 选中 Actor 的标签改成 ENV_{类别}_{变体}_{序号}，按网格名推导。 */
	static int32 FixSelectedActorLabels(TArray<FString>& OutRenamed);

private:
	static bool IsAllDigits(const FString& Token);
};
