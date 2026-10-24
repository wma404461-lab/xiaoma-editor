#include "PonyNamingService.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/IAssetRegistry.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Selection.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

namespace
{
	bool IsJunkToken(const FString& Token)
	{
		const FString Lower = Token.ToLower();
		static const TCHAR* Junk[] = { TEXT("final"), TEXT("copy"), TEXT("new"), TEXT("temp"), TEXT("last"), TEXT("ok"), TEXT("Copy"), TEXT("Final") };
		for (const TCHAR* Word : Junk)
		{
			if (Lower == Word)
			{
				return true;
			}
		}
		// v2 / V03 This version number type
		if (Lower.Len() >= 2 && Lower[0] == 'v' && Lower.RightChop(1).IsNumeric())
		{
			return true;
		}
		return false;
	}

	FString ToPascal(const FString& Token)
	{
		if (Token.IsEmpty())
		{
			return Token;
		}
		FString Result = Token.ToLower();
		Result[0] = FChar::ToUpper(Result[0]);
		return Result;
	}
}

FString FPonyNamingService::GetPrefixForClass(const FString& ClassName)
{
	if (ClassName == TEXT("StaticMesh")) { return TEXT("SM_"); }
	if (ClassName == TEXT("SkeletalMesh")) { return TEXT("SK_"); }
	if (ClassName == TEXT("Material")) { return TEXT("M_"); }
	if (ClassName == TEXT("MaterialInstanceConstant")) { return TEXT("MI_"); }
	if (ClassName == TEXT("Texture2D") || ClassName == TEXT("TextureCube")) { return TEXT("T_"); }
	if (ClassName == TEXT("Blueprint")) { return TEXT("BP_"); }
	if (ClassName == TEXT("NiagaraSystem")) { return TEXT("NS_"); }
	if (ClassName == TEXT("SoundWave")) { return TEXT("S_"); }
	if (ClassName.Contains(TEXT("DataAsset"))) { return TEXT("DA_"); }
	return FString();
}

bool FPonyNamingService::IsAllDigits(const FString& Token)
{
	return !Token.IsEmpty() && Token.IsNumeric();
}

FString FPonyNamingService::SuggestName(const FString& CurrentName, const FString& ClassName, FString& OutReason)
{
	OutReason.Reset();

	const FString Prefix = GetPrefixForClass(ClassName);
	if (Prefix.IsEmpty())
	{
		return CurrentName;
	}

	bool bNonAscii = false;
	for (const TCHAR Character : CurrentName)
	{
		if (Character > 127)
		{
			bNonAscii = true;
			break;
		}
	}

	if (CurrentName.Contains(TEXT(" ")) || CurrentName.Contains(TEXT("-")) || CurrentName.Contains(TEXT(".")))
	{
		OutReason = TEXT("Contains spaces / hyphens / dots");
	}
	else if (bNonAscii)
	{
		OutReason = TEXT("Contains Chinese or full-width characters");
	}
	else if (!CurrentName.StartsWith(Prefix))
	{
		OutReason = FString::Printf(TEXT("Missing %s prefix"), *Prefix);
	}
	else if (CurrentName.Contains(TEXT("(")) || CurrentName.Contains(TEXT(")")))
	{
		OutReason = TEXT("Contains suffixes like (1)");
	}

	FString Core = CurrentName;
	if (Core.StartsWith(Prefix))
	{
		Core.RightChopInline(Prefix.Len());
	}

	FString Normalized = Core;
	Normalized.ReplaceInline(TEXT("-"), TEXT("_"));
	Normalized.ReplaceInline(TEXT(" "), TEXT("_"));
	Normalized.ReplaceInline(TEXT("."), TEXT("_"));
	Normalized.ReplaceInline(TEXT("("), TEXT("_"));
	Normalized.ReplaceInline(TEXT(")"), TEXT("_"));

	TArray<FString> Tokens;
	Normalized.ParseIntoArray(Tokens, TEXT("_"), true);

	TArray<FString> Kept;
	bool bDroppedJunk = false;
	for (const FString& Token : Tokens)
	{
		if (IsJunkToken(Token))
		{
			bDroppedJunk = true;
			continue;
		}
		Kept.Add(Token);
	}
	if (bDroppedJunk && OutReason.IsEmpty())
	{
		OutReason = TEXT("Contains residual _final/_copy/version tags");
	}

	FString Index;
	if (Kept.Num() > 1 && IsAllDigits(Kept.Last()))
	{
		Index = Kept.Pop();
		if (Index.Len() < 2 && OutReason.IsEmpty())
		{
			OutReason = TEXT("Sequence number not zero-padded");
		}
	}

	FString Suffix;
	if (Kept.Num() > 1)
	{
		const FString Last = Kept.Last().ToUpper();
		if (Last == TEXT("D") || Last == TEXT("N") || Last == TEXT("R") || Last == TEXT("H") || Last == TEXT("AO"))
		{
			Suffix = Last;
			Kept.Pop();
		}
	}

	FString Pascal;
	for (const FString& Token : Kept)
	{
		Pascal += ToPascal(Token);
	}
	if (Pascal.IsEmpty())
	{
		Pascal = TEXT("Unnamed");
	}

	FString Suggested = Prefix + Pascal;
	if (!Index.IsEmpty())
	{
		Suggested += FString::Printf(TEXT("_%02d"), FCString::Atoi(*Index));
	}
	if (!Suffix.IsEmpty())
	{
		Suggested += TEXT("_") + Suffix;
	}

	if (Suggested == CurrentName && OutReason.IsEmpty())
	{
		return CurrentName;
	}
	return Suggested;
}

void FPonyNamingService::ScanAssets(const TArray<FAssetData>& Assets, TArray<FPonyNamingIssue>& OutIssues)
{
	for (const FAssetData& Asset : Assets)
	{
		const FString ClassName = Asset.AssetClassPath.GetAssetName().ToString();
		if (GetPrefixForClass(ClassName).IsEmpty() || Asset.AssetName.IsNone())
		{
			continue;
		}

		FString Reason;
		const FString Suggested = SuggestName(Asset.AssetName.ToString(), ClassName, Reason);
		if (!Reason.IsEmpty() || Suggested != Asset.AssetName.ToString())
		{
			FPonyNamingIssue Issue;
			Issue.Asset = Asset;
			Issue.CurrentName = Asset.AssetName.ToString();
			Issue.SuggestedName = Suggested;
			Issue.Reason = Reason.IsEmpty() ? TEXT("Name does not conform to standards") : Reason;
			Issue.Severity = Reason.Contains(TEXT("Prefix")) || Reason.Contains(TEXT("Chinese")) || Reason.Contains(TEXT("Space")) ? 2 : 1;
			OutIssues.Add(Issue);
		}
	}
}

void FPonyNamingService::ScanPath(const FString& ContentPath, TArray<FPonyNamingIssue>& OutIssues, int32& OutScanned)
{
	OutScanned = 0;

	FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	Module.Get().GetAssetsByPath(FName(*ContentPath), Assets, true);
	OutScanned = Assets.Num();

	ScanAssets(Assets, OutIssues);
}

int32 FPonyNamingService::FixSelectedActorLabels(TArray<FString>& OutRenamed)
{
	if (!GEditor)
	{
		return 0;
	}

	TMap<FString, int32> Counters;
	int32 Changed = 0;

	USelection* Selection = GEditor->GetSelectedActors();
	if (!Selection)
	{
		return 0;
	}

	for (FSelectionIterator It(*Selection); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		if (!Actor)
		{
			continue;
		}

		FString MeshName;
		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetStaticMesh())
			{
				MeshName = Component->GetStaticMesh()->GetName();
				break;
			}
		}

		if (MeshName.IsEmpty())
		{
			continue;
		}

		// Already ENV_ Treat as already processed at start to avoid incrementing sequence number on each click.
		if (Actor->GetActorLabel().StartsWith(TEXT("ENV_")))
		{
			continue;
		}

		if (MeshName.StartsWith(TEXT("SM_")))
		{
			MeshName.RightChopInline(3);
		}

		TArray<FString> Tokens;
		MeshName.ParseIntoArray(Tokens, TEXT("_"), true);

		FString Key = TEXT("ENV");
		for (const FString& Token : Tokens)
		{
			if (IsJunkToken(Token))
			{
				continue;
			}
			Key += TEXT("_");
			Key += ToPascal(Token);
		}

		int32& Counter = Counters.FindOrAdd(Key);
		++Counter;

		const FString NewLabel = FString::Printf(TEXT("%s_%02d"), *Key, Counter);
		if (Actor->GetActorLabel() != NewLabel)
		{
			const FString OldLabel = Actor->GetActorLabel();
			Actor->SetActorLabel(NewLabel);
			OutRenamed.Add(FString::Printf(TEXT("%s -> %s"), *OldLabel, *NewLabel));
			++Changed;
		}
	}

	return Changed;
}

int32 FPonyNamingService::ApplyFixes(const TArray<FPonyNamingIssue>& Issues, TArray<FString>& OutRenamed, TArray<FString>& OutFailed)
{
	if (Issues.Num() == 0)
	{
		return 0;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	int32 Renamed = 0;
	for (const FPonyNamingIssue& Issue : Issues)
	{
		UObject* AssetObject = Issue.Asset.GetAsset();
		if (!AssetObject || Issue.SuggestedName.IsEmpty() || Issue.SuggestedName == Issue.CurrentName)
		{
			continue;
		}

		const FString PackagePath = Issue.Asset.PackagePath.ToString();
		TArray<FAssetRenameData> RenameData;
		RenameData.Emplace(AssetObject, PackagePath, Issue.SuggestedName);

		if (AssetTools.RenameAssets(RenameData))
		{
			++Renamed;
			OutRenamed.Add(FString::Printf(TEXT("%s -> %s"), *Issue.CurrentName, *Issue.SuggestedName));
		}
		else
		{
			OutFailed.Add(FString::Printf(TEXT("%s -> %s"), *Issue.CurrentName, *Issue.SuggestedName));
		}
	}

	return Renamed;
}
