#include "PonyEditImportLibrary.h"

#include "PonyNamingService.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ObjectTools.h"
#include "AssetImportTask.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	bool IsNormalSuffix(const FString& Name)
	{
		return Name.EndsWith(TEXT("_N")) || Name.EndsWith(TEXT("_n"));
	}

	bool IsLinearSuffix(const FString& Name)
	{
		return Name.EndsWith(TEXT("_R")) || Name.EndsWith(TEXT("_AO")) || Name.EndsWith(TEXT("_H"));
	}

	FString WriteJson(const TSharedRef<FJsonObject>& Root)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Root, Writer);
		return Output;
	}

	FString MakeArray(const TArray<FString>& Items, const TSharedRef<FJsonObject>& Root, const TCHAR* Field)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		for (const FString& Item : Items)
		{
			Values.Add(MakeShared<FJsonValueString>(Item));
		}
		Root->SetArrayField(Field, Values);
		return FString();
	}
}

FString UPonyEditImportLibrary::ImportTextures(const FString& SourceDir, const FString& DestinationPath)
{
	TArray<FString> Imported;
	TArray<FString> Renamed;
	TArray<FString> Failed;

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(SourceDir, TEXT("*.png")), true, false);
	if (Files.Num() == 0)
	{
		IFileManager::Get().FindFiles(Files, *FPaths::Combine(SourceDir, TEXT("*.tga")), true, false);
	}

	if (Files.Num() == 0)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetStringField(TEXT("error"), TEXT("目录里没有 png/tga"));
		return WriteJson(Root);
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	TArray<UAssetImportTask*> Tasks;
	for (const FString& File : Files)
	{
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = FPaths::Combine(SourceDir, File);
		Task->DestinationPath = DestinationPath;
		Task->bAutomated = true;
		Task->bReplaceExisting = true;
		Task->bSave = true;
		Task->Factory = NewObject<UTextureFactory>(Task);
		Tasks.Add(Task);
	}

	AssetTools.ImportAssetTasks(Tasks);

	TArray<UPackage*> PackagesToSave;
	TArray<FAssetRenameData> Renames;
	for (UAssetImportTask* Task : Tasks)
	{
		for (UObject* Object : Task->GetObjects())
		{
			UTexture2D* Texture = Cast<UTexture2D>(Object);
			if (!Texture)
			{
				continue;
			}

			const FString SourceName = FPaths::GetBaseFilename(Task->Filename);
			Texture->SRGB = !IsNormalSuffix(SourceName) && !IsLinearSuffix(SourceName);

			if (IsNormalSuffix(SourceName))
			{
				Texture->CompressionSettings = TC_Normalmap;
				Texture->LODGroup = TEXTUREGROUP_WorldNormalMap;
			}
			else
			{
				Texture->CompressionSettings = TC_Default;
				Texture->LODGroup = IsLinearSuffix(SourceName) ? TEXTUREGROUP_World : TEXTUREGROUP_World;
			}

			Texture->PostEditChange();
			Imported.Add(Texture->GetPathName());

			// 命名规则：T_ + PascalCase，保留 _N/_D/_R 通道后缀。
			FString Reason;
			const FString Suggested = FPonyNamingService::SuggestName(Texture->GetName(), TEXT("Texture2D"), Reason);
			if (!Suggested.IsEmpty() && Suggested != Texture->GetName())
			{
				// 覆盖语义：目标名已存在就先删旧的，避免改名撞名。
				FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				const FSoftObjectPath TargetPath(FString::Printf(TEXT("%s.%s"), *DestinationPath, *Suggested));
				const FAssetData Existing = RegistryModule.Get().GetAssetByObjectPath(TargetPath);
				if (Existing.IsValid())
				{
					if (UObject* OldAsset = Existing.GetAsset())
					{
						ObjectTools::DeleteSingleObject(OldAsset, true);
					}
				}

				FAssetRenameData RenameData(Texture, DestinationPath, Suggested);
				Renames.Add(RenameData);
				Renamed.Add(FString::Printf(TEXT("%s -> %s"), *SourceName, *Suggested));
			}

			if (UPackage* Package = Texture->GetOutermost())
			{
				PackagesToSave.AddUnique(Package);
			}
		}
	}

	if (Renames.Num() > 0)
	{
		AssetTools.RenameAssets(Renames);
	}

	if (PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("fileCount"), Files.Num());
	Root->SetNumberField(TEXT("importedCount"), Imported.Num());
	MakeArray(Imported, Root, TEXT("imported"));
	MakeArray(Renamed, Root, TEXT("renamed"));
	MakeArray(Failed, Root, TEXT("failed"));
	return WriteJson(Root);
}
