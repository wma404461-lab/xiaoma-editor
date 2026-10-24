#include "PonyEditMaterialLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* GMasterRoot = TEXT("/Game/PonyEdit/Materials/Masters");
	const TCHAR* GInstanceRoot = TEXT("/Game/PonyEdit/Materials/Instances");
	const TCHAR* GMasterPrefix = TEXT("PE_MM_Std_");

	enum class EPonyParamType : uint8
	{
		Texture,
		Vector,
		Scalar,
		StaticSwitch
	};

	struct FPonyParamSpec
	{
		FName Name;
		EPonyParamType Type = EPonyParamType::Scalar;
		FLinearColor VectorValue = FLinearColor::White;
		float ScalarValue = 0.f;
		bool BoolValue = false;
	};

	struct FPonyMasterSpec
	{
		FName Name;
		FString Usage;
		TArray<FPonyParamSpec> Params;
	};

	struct FPonyCreateResult
	{
		bool bOk = false;
		bool bSaved = false;
		FString Error;
		FString AssetName;
		FString AssetPath;
		bool bReused = false;
		int32 ParamCount = 0;
		int32 ParamsApplied = 0;
		TArray<FString> Missing;
		UMaterialInstanceConstant* Instance = nullptr;
	};

	struct FPonyApplyResult
	{
		int32 ActorCount = 0;
		int32 ComponentCount = 0;
		int32 SlotCount = 0;
	};

	FPonyParamSpec MakeTextureParam(const TCHAR* Name)
	{
		FPonyParamSpec Spec;
		Spec.Name = Name;
		Spec.Type = EPonyParamType::Texture;
		return Spec;
	}

	FPonyParamSpec MakeVectorParam(const TCHAR* Name, const FLinearColor& Value)
	{
		FPonyParamSpec Spec;
		Spec.Name = Name;
		Spec.Type = EPonyParamType::Vector;
		Spec.VectorValue = Value;
		return Spec;
	}

	FPonyParamSpec MakeScalarParam(const TCHAR* Name, float Value)
	{
		FPonyParamSpec Spec;
		Spec.Name = Name;
		Spec.Type = EPonyParamType::Scalar;
		Spec.ScalarValue = Value;
		return Spec;
	}

	FPonyParamSpec MakeSwitchParam(const TCHAR* Name, bool Value)
	{
		FPonyParamSpec Spec;
		Spec.Name = Name;
		Spec.Type = EPonyParamType::StaticSwitch;
		Spec.BoolValue = Value;
		return Spec;
	}

	const TCHAR* ParamTypeToString(EPonyParamType Type)
	{
		switch (Type)
		{
		case EPonyParamType::Texture: return TEXT("Texture");
		case EPonyParamType::Vector: return TEXT("Vector");
		case EPonyParamType::Scalar: return TEXT("Scalar");
		case EPonyParamType::StaticSwitch: return TEXT("StaticSwitch");
		default: return TEXT("Unknown");
		}
	}

	TArray<FPonyParamSpec> MakeCoreParams()
	{
		TArray<FPonyParamSpec> Params;
		Params.Add(MakeTextureParam(TEXT("BaseColor")));
		Params.Add(MakeVectorParam(TEXT("BaseColorTint"), FLinearColor(1.f, 1.f, 1.f, 1.f)));
		Params.Add(MakeScalarParam(TEXT("Roughness"), 0.5f));
		Params.Add(MakeScalarParam(TEXT("Metallic"), 0.f));
		Params.Add(MakeTextureParam(TEXT("Normal")));
		Params.Add(MakeScalarParam(TEXT("NormalStrength"), 1.f));
		Params.Add(MakeTextureParam(TEXT("AO")));
		Params.Add(MakeTextureParam(TEXT("DetailNormal")));
		Params.Add(MakeScalarParam(TEXT("DetailScale"), 4.f));
		Params.Add(MakeSwitchParam(TEXT("UseVertexColor"), false));
		return Params;
	}

	TArray<FPonyMasterSpec> BuildContract()
	{
		TArray<FPonyMasterSpec> Specs;

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Opaque");
			Spec.Usage = TEXT("General Opaque");
			Spec.Params = MakeCoreParams();
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Masked");
			Spec.Usage = TEXT("Leaves / Wire mesh");
			Spec.Params = MakeCoreParams();
			Spec.Params.Add(MakeScalarParam(TEXT("OpacityMask"), 1.f));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Foliage");
			Spec.Usage = TEXT("Vegetation");
			Spec.Params = MakeCoreParams();
			Spec.Params.Add(MakeScalarParam(TEXT("OpacityMask"), 1.f));
			Spec.Params.Add(MakeScalarParam(TEXT("WindIntensity"), 0.3f));
			Spec.Params.Add(MakeScalarParam(TEXT("WindSpeed"), 1.f));
			Spec.Params.Add(MakeVectorParam(TEXT("SSSColor"), FLinearColor(0.25f, 0.12f, 0.05f, 1.f)));
			Spec.Params.Add(MakeScalarParam(TEXT("SSSAmount"), 0.5f));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Transparent");
			Spec.Usage = TEXT("Glass / Water surface");
			Spec.Params = MakeCoreParams();
			Spec.Params.Add(MakeScalarParam(TEXT("Opacity"), 0.5f));
			Spec.Params.Add(MakeScalarParam(TEXT("FresnelPower"), 3.f));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Decal");
			Spec.Usage = TEXT("Decals");
			Spec.Params = MakeCoreParams();
			Spec.Params.Add(MakeScalarParam(TEXT("Opacity"), 1.f));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_Emissive");
			Spec.Usage = TEXT("LED strips / Screens");
			Spec.Params.Add(MakeTextureParam(TEXT("BaseColor")));
			Spec.Params.Add(MakeVectorParam(TEXT("BaseColorTint"), FLinearColor(1.f, 1.f, 1.f, 1.f)));
			Spec.Params.Add(MakeScalarParam(TEXT("EmissiveIntensity"), 1.f));
			Spec.Params.Add(MakeSwitchParam(TEXT("UseVertexColor"), false));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_TerrainBlend");
			Spec.Usage = TEXT("Terrain 3–4 layers");
			for (int32 Layer = 0; Layer < 4; ++Layer)
			{
				Spec.Params.Add(MakeTextureParam(*FString::Printf(TEXT("Layer%dColor"), Layer)));
				Spec.Params.Add(MakeTextureParam(*FString::Printf(TEXT("Layer%dNormal"), Layer)));
				Spec.Params.Add(MakeScalarParam(*FString::Printf(TEXT("Layer%dRoughness"), Layer), 0.5f));
			}
			Spec.Params.Add(MakeVectorParam(TEXT("BaseColorTint"), FLinearColor(1.f, 1.f, 1.f, 1.f)));
			Spec.Params.Add(MakeSwitchParam(TEXT("UseRVT"), false));
			Specs.Add(Spec);
		}

		{
			FPonyMasterSpec Spec;
			Spec.Name = TEXT("PE_MM_Std_VertexBlend");
			Spec.Usage = TEXT("Modular Wall / Prop");
			Spec.Params = MakeCoreParams();
			Spec.Params.Add(MakeVectorParam(TEXT("SecondaryColor"), FLinearColor(0.35f, 0.30f, 0.25f, 1.f)));
			Specs.Add(Spec);
		}

		return Specs;
	}

	const TArray<FPonyMasterSpec>& GetContract()
	{
		static const TArray<FPonyMasterSpec> Specs = BuildContract();
		return Specs;
	}

	const FPonyMasterSpec* FindSpec(const FName& MasterName)
	{
		for (const FPonyMasterSpec& Spec : GetContract())
		{
			if (Spec.Name == MasterName)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	FString GetMasterAssetPath(const FName& MasterName)
	{
		return FString::Printf(TEXT("%s/%s"), GMasterRoot, *MasterName.ToString());
	}

	FString WriteJson(const TSharedRef<FJsonObject>& Root)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Root, Writer);
		return Output;
	}

	FString ErrorJson(const FString& Error)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), false);
		Root->SetStringField(TEXT("error"), Error);
		return WriteJson(Root);
	}

	void SetStringArrayField(const TSharedRef<FJsonObject>& Root, const TCHAR* Field, const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> Array;
		for (const FString& Value : Values)
		{
			Array.Add(MakeShared<FJsonValueString>(Value));
		}
		Root->SetArrayField(Field, Array);
	}

	UObject* LoadAssetObject(const FString& Path)
	{
		FString ObjectPath = Path;
		if (!ObjectPath.Contains(TEXT(".")))
		{
			ObjectPath += TEXT(".");
			ObjectPath += FPackageName::GetShortName(Path);
		}
		return LoadObject<UObject>(nullptr, *ObjectPath);
	}

	FString NormalizeInstanceName(const FString& InName)
	{
		FString Name = InName;
		Name.TrimStartAndEndInline();
		Name.ReplaceInline(TEXT(" "), TEXT("_"));
		Name.ReplaceInline(TEXT("-"), TEXT("_"));
		Name.ReplaceInline(TEXT("."), TEXT("_"));
		Name.ReplaceInline(TEXT("("), TEXT("_"));
		Name.ReplaceInline(TEXT(")"), TEXT("_"));
		if (!Name.StartsWith(TEXT("MI_")))
		{
			Name = TEXT("MI_") + Name;
		}
		return Name;
	}

	void GatherMaterialParams(const UMaterial* Material, TMap<FName, EPonyParamType>& OutParams)
	{
		if (!Material)
		{
			return;
		}

		for (const TObjectPtr<UMaterialExpression>& Expression : Material->GetExpressions())
		{
			UMaterialExpression* Expr = Expression.Get();
			if (!Expr)
			{
				continue;
			}

			if (const UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr))
			{
				if (!TextureParam->ParameterName.IsNone())
				{
					OutParams.Add(TextureParam->ParameterName, EPonyParamType::Texture);
				}
			}
			else if (const UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
			{
				if (!VectorParam->ParameterName.IsNone())
				{
					OutParams.Add(VectorParam->ParameterName, EPonyParamType::Vector);
				}
			}
			else if (const UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
			{
				if (!ScalarParam->ParameterName.IsNone())
				{
					OutParams.Add(ScalarParam->ParameterName, EPonyParamType::Scalar);
				}
			}
			else if (const UMaterialExpressionStaticSwitchParameter* SwitchParam = Cast<UMaterialExpressionStaticSwitchParameter>(Expr))
			{
				if (!SwitchParam->ParameterName.IsNone())
				{
					OutParams.Add(SwitchParam->ParameterName, EPonyParamType::StaticSwitch);
				}
			}
		}
	}

	void AddParamList(const FPonyMasterSpec& Spec, const TMap<FName, EPonyParamType>& Found, const TSharedRef<FJsonObject>& Root)
	{
		TArray<TSharedPtr<FJsonValue>> ParamArray;
		for (const FPonyParamSpec& Param : Spec.Params)
		{
			const TSharedRef<FJsonObject> ParamObject = MakeShared<FJsonObject>();
			ParamObject->SetStringField(TEXT("name"), Param.Name.ToString());
			ParamObject->SetStringField(TEXT("type"), ParamTypeToString(Param.Type));
			ParamObject->SetBoolField(TEXT("found"), Found.Contains(Param.Name));
			ParamArray.Add(MakeShared<FJsonValueObject>(ParamObject));
		}
		Root->SetArrayField(TEXT("params"), ParamArray);
	}

	void BuildMasterReport(const FPonyMasterSpec& Spec, const TSharedRef<FJsonObject>& Root)
	{
		const FString MasterPath = GetMasterAssetPath(Spec.Name);
		UMaterial* Master = Cast<UMaterial>(LoadAssetObject(MasterPath));

		TMap<FName, EPonyParamType> Found;
		GatherMaterialParams(Master, Found);

		TArray<FString> Missing;
		for (const FPonyParamSpec& Param : Spec.Params)
		{
			if (!Found.Contains(Param.Name))
			{
				Missing.Add(Param.Name.ToString());
			}
		}

		TArray<FString> Extra;
		for (const TPair<FName, EPonyParamType>& Pair : Found)
		{
			const bool bExpected = Spec.Params.ContainsByPredicate([&Pair](const FPonyParamSpec& Param)
			{
				return Param.Name == Pair.Key;
			});
			if (!bExpected)
			{
				Extra.Add(Pair.Key.ToString());
			}
		}

		Root->SetStringField(TEXT("name"), Spec.Name.ToString());
		Root->SetStringField(TEXT("usage"), Spec.Usage);
		Root->SetStringField(TEXT("path"), MasterPath);
		Root->SetStringField(TEXT("objectPath"), Master ? Master->GetPathName() : FString());
		Root->SetBoolField(TEXT("exists"), Master != nullptr);
		Root->SetNumberField(TEXT("paramCount"), Found.Num());
		Root->SetNumberField(TEXT("expectedCount"), Spec.Params.Num());
		SetStringArrayField(Root, TEXT("missing"), Missing);
		SetStringArrayField(Root, TEXT("extra"), Extra);
		AddParamList(Spec, Found, Root);

		const bool bOk = Master != nullptr && Missing.Num() == 0;
		Root->SetBoolField(TEXT("ok"), bOk);
	}

	TMap<FName, int32> CountInstancesByParent()
	{
		TMap<FName, int32> Counts;

		FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FARFilter Filter;
		Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;
		Filter.PackagePaths.Add(FName(TEXT("/Game")));
		Filter.bRecursivePaths = true;

		TArray<FAssetData> Assets;
		Module.Get().GetAssets(Filter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			FString ParentPath;
			if (Asset.GetTagValue(FName(TEXT("Parent")), ParentPath) && !ParentPath.IsEmpty())
			{
				// 5.6 of Parent tag Exported text: /Script/Engine.Material'/Game/.../M_Name.M_Name'
				// Extract object path from single quotes, then get short name.
				FString CleanParent = ParentPath;
				int32 QuoteStart = INDEX_NONE;
				if (CleanParent.FindChar(TEXT('\''), QuoteStart))
				{
					const FString AfterQuote = CleanParent.Mid(QuoteStart + 1);
					int32 QuoteEnd = INDEX_NONE;
					if (AfterQuote.FindChar(TEXT('\''), QuoteEnd))
					{
						CleanParent = AfterQuote.Left(QuoteEnd);
					}
				}

				const FString ParentName = FPackageName::GetShortName(CleanParent);
				if (!ParentName.IsEmpty() && FindSpec(FName(*ParentName)))
				{
					Counts.FindOrAdd(FName(*ParentName))++;
					continue;
				}
			}

			if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Asset.GetAsset()))
			{
				if (UMaterialInterface* Parent = Instance->Parent.Get())
				{
					Counts.FindOrAdd(Parent->GetFName())++;
				}
			}
		}

		return Counts;
	}

	void ApplyParamToInstance(UMaterialInstanceConstant* Instance, const FPonyParamSpec& Param, int32& InOutApplied)
	{
		if (!Instance)
		{
			return;
		}

		const FMaterialParameterInfo Info(Param.Name);
		switch (Param.Type)
		{
		case EPonyParamType::Vector:
			Instance->SetVectorParameterValueEditorOnly(Info, Param.VectorValue);
			++InOutApplied;
			break;
		case EPonyParamType::Scalar:
			Instance->SetScalarParameterValueEditorOnly(Info, Param.ScalarValue);
			++InOutApplied;
			break;
		case EPonyParamType::StaticSwitch:
			Instance->SetStaticSwitchParameterValueEditorOnly(Info, Param.BoolValue);
			++InOutApplied;
			break;
		case EPonyParamType::Texture:
			// Texture not overwritten, inherits default texture from parent material; MI Override only scalar values/Vector/Switch.
			break;
		default:
			break;
		}
	}

	bool CreateInstanceInternal(const FString& MasterPath, const FString& PackagePath, const FString& AssetName, FPonyCreateResult& OutResult)
	{
		OutResult = FPonyCreateResult();

		FString FinalPackagePath = PackagePath;
		FinalPackagePath.TrimStartAndEndInline();
		if (FinalPackagePath.IsEmpty())
		{
			FinalPackagePath = GInstanceRoot;
		}
		if (!FPackageName::IsValidLongPackageName(FinalPackagePath))
		{
			OutResult.Error = FString::Printf(TEXT("Invalid package path: %s"), *FinalPackagePath);
			return false;
		}

		const FString MasterName = FPackageName::GetShortName(MasterPath);
		const FPonyMasterSpec* Spec = FindSpec(FName(*MasterName));
		if (!Spec)
		{
			OutResult.Error = FString::Printf(TEXT("Parent material not in contract table: %s"), *MasterName);
			return false;
		}

		UMaterial* Master = Cast<UMaterial>(LoadAssetObject(GetMasterAssetPath(Spec->Name)));
		if (!Master)
		{
			OutResult.Error = FString::Printf(TEXT("Parent material does not exist: %s"), *GetMasterAssetPath(Spec->Name));
			return false;
		}

		TMap<FName, EPonyParamType> Found;
		GatherMaterialParams(Master, Found);
		for (const FPonyParamSpec& Param : Spec->Params)
		{
			if (!Found.Contains(Param.Name))
			{
				OutResult.Missing.Add(Param.Name.ToString());
			}
		}
		if (OutResult.Missing.Num() > 0)
		{
			OutResult.Error = TEXT("Parent material contract is incomplete, refusing to create MI");
			return false;
		}

		const FString FinalName = NormalizeInstanceName(AssetName);
		if (FinalName.IsEmpty() || FinalName == TEXT("MI_"))
		{
			OutResult.Error = TEXT("MI name is empty");
			return false;
		}

		const FString FullPath = FinalPackagePath + TEXT("/") + FinalName;
		UObject* ExistingObject = LoadAssetObject(FullPath);
		UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(ExistingObject);
		if (ExistingObject && !Instance)
		{
			OutResult.Error = FString::Printf(TEXT("An asset with the same name already exists and is not an MI: %s"), *FullPath);
			return false;
		}

		if (Instance)
		{
			Instance->ClearParameterValuesEditorOnly();
			OutResult.bReused = true;
		}
		else
		{
			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
			Instance = Cast<UMaterialInstanceConstant>(AssetTools.CreateAsset(
				FinalName, FinalPackagePath, UMaterialInstanceConstant::StaticClass(), Factory));
			if (!Instance)
			{
				OutResult.Error = FString::Printf(TEXT("Failed to create MI: %s"), *FullPath);
				return false;
			}
		}

		Instance->SetParentEditorOnly(Master);
		for (const FPonyParamSpec& Param : Spec->Params)
		{
			ApplyParamToInstance(Instance, Param, OutResult.ParamsApplied);
		}
		Instance->PostEditChange();

		if (UPackage* Package = Instance->GetOutermost())
		{
			Package->MarkPackageDirty();
			TArray<UPackage*> PackagesToSave;
			PackagesToSave.Add(Package);
			OutResult.bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
		}

		OutResult.bOk = OutResult.bSaved;
		if (!OutResult.bSaved)
		{
			OutResult.Error = TEXT("MI already created in memory, but failed to save to disk");
		}
		OutResult.AssetName = FinalName;
		OutResult.AssetPath = FullPath;
		OutResult.ParamCount = Spec->Params.Num();
		OutResult.Instance = Instance;
		return OutResult.bOk;
	}

	TSharedRef<FJsonObject> SerializeCreateResult(const FPonyCreateResult& Result)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), Result.bOk);
		if (!Result.bOk)
		{
			Root->SetStringField(TEXT("error"), Result.Error);
		}
		Root->SetStringField(TEXT("assetName"), Result.AssetName);
		Root->SetStringField(TEXT("assetPath"), Result.AssetPath);
		Root->SetBoolField(TEXT("reused"), Result.bReused);
		Root->SetBoolField(TEXT("saved"), Result.bSaved);
		Root->SetNumberField(TEXT("paramCount"), Result.ParamCount);
		Root->SetNumberField(TEXT("paramsApplied"), Result.ParamsApplied);
		SetStringArrayField(Root, TEXT("missing"), Result.Missing);
		return Root;
	}

	bool FindSelectedMeshName(FString& OutMeshName)
	{
		if (!GEditor)
		{
			return false;
		}

		USelection* Selection = GEditor->GetSelectedActors();
		if (!Selection)
		{
			return false;
		}

		for (FSelectionIterator It(*Selection); It; ++It)
		{
			AActor* Actor = Cast<AActor>(*It);
			if (!Actor)
			{
				continue;
			}

			TArray<UStaticMeshComponent*> Components;
			Actor->GetComponents<UStaticMeshComponent>(Components);
			for (UStaticMeshComponent* Component : Components)
			{
				if (Component && Component->GetStaticMesh())
				{
					OutMeshName = Component->GetStaticMesh()->GetName();
					return true;
				}
			}
		}

		return false;
	}

	void CollectMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutComponents)
	{
		if (!Actor)
		{
			return;
		}

		TArray<UStaticMeshComponent*> StaticComponents;
		Actor->GetComponents<UStaticMeshComponent>(StaticComponents);
		for (UStaticMeshComponent* Component : StaticComponents)
		{
			OutComponents.AddUnique(Component);
		}

		TArray<USkeletalMeshComponent*> SkeletalComponents;
		Actor->GetComponents<USkeletalMeshComponent>(SkeletalComponents);
		for (USkeletalMeshComponent* Component : SkeletalComponents)
		{
			OutComponents.AddUnique(Component);
		}
	}

	FPonyApplyResult ApplyMaterialToSelection(UMaterialInterface* Material, int32 SlotIndex)
	{
		FPonyApplyResult Result;
		if (!GEditor || !Material)
		{
			return Result;
		}

		USelection* Selection = GEditor->GetSelectedActors();
		if (!Selection)
		{
			return Result;
		}

		const FScopedTransaction Transaction(FText::FromString(TEXT("Xiaoma Editor: Apply Material")));

		for (FSelectionIterator It(*Selection); It; ++It)
		{
			AActor* Actor = Cast<AActor>(*It);
			if (!Actor)
			{
				continue;
			}

			TArray<UMeshComponent*> Components;
			CollectMeshComponents(Actor, Components);
			if (Components.Num() == 0)
			{
				continue;
			}

			Actor->Modify();
			++Result.ActorCount;

			for (UMeshComponent* Component : Components)
			{
				const int32 NumSlots = Component->GetNumMaterials();
				if (NumSlots <= 0)
				{
					continue;
				}

				Component->Modify();
				++Result.ComponentCount;

				if (SlotIndex < 0)
				{
					for (int32 Index = 0; Index < NumSlots; ++Index)
					{
						Component->SetMaterial(Index, Material);
						++Result.SlotCount;
					}
				}
				else if (SlotIndex < NumSlots)
				{
					Component->SetMaterial(SlotIndex, Material);
					++Result.SlotCount;
				}
			}
		}

		if (Result.SlotCount > 0)
		{
			GEditor->RedrawLevelEditingViewports(true);
		}

		return Result;
	}
}

FString UPonyEditMaterialLibrary::GetRack()
{
	const TArray<FPonyMasterSpec>& Specs = GetContract();
	const TMap<FName, int32> InstanceCounts = CountInstancesByParent();

	int32 ExistingCount = 0;
	int32 HealthyCount = 0;
	TArray<TSharedPtr<FJsonValue>> MasterArray;

	for (const FPonyMasterSpec& Spec : Specs)
	{
		const TSharedRef<FJsonObject> MasterObject = MakeShared<FJsonObject>();
		BuildMasterReport(Spec, MasterObject);
		MasterObject->SetNumberField(TEXT("miCount"), InstanceCounts.FindRef(Spec.Name));
		MasterArray.Add(MakeShared<FJsonValueObject>(MasterObject));

		if (MasterObject->GetBoolField(TEXT("exists")))
		{
			++ExistingCount;
		}
		if (MasterObject->GetBoolField(TEXT("ok")))
		{
			++HealthyCount;
		}
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("ok"), HealthyCount == Specs.Num());
	Root->SetStringField(TEXT("mastersPath"), GMasterRoot);
	Root->SetNumberField(TEXT("masterCount"), Specs.Num());
	Root->SetNumberField(TEXT("existingCount"), ExistingCount);
	Root->SetNumberField(TEXT("healthyCount"), HealthyCount);
	Root->SetArrayField(TEXT("masters"), MasterArray);
	return WriteJson(Root);
}

FString UPonyEditMaterialLibrary::ValidateMaster(const FString& MasterPath)
{
	const FString MasterName = FPackageName::GetShortName(MasterPath);
	const FPonyMasterSpec* Spec = FindSpec(FName(*MasterName));

	if (!Spec)
	{
		UMaterial* Material = Cast<UMaterial>(LoadAssetObject(MasterPath));
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), Material != nullptr);
		Root->SetBoolField(TEXT("knownContract"), false);
		Root->SetStringField(TEXT("path"), MasterPath);
		Root->SetStringField(TEXT("error"), Material ? TEXT("Parent material not in contract table") : TEXT("Parent material does not exist"));
		if (Material)
		{
			TMap<FName, EPonyParamType> Found;
			GatherMaterialParams(Material, Found);
			Root->SetNumberField(TEXT("paramCount"), Found.Num());
			Root->SetArrayField(TEXT("params"), TArray<TSharedPtr<FJsonValue>>());
		}
		return WriteJson(Root);
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	BuildMasterReport(*Spec, Root);
	Root->SetBoolField(TEXT("knownContract"), true);
	return WriteJson(Root);
}

FString UPonyEditMaterialLibrary::CreateInstance(const FString& MasterPath, const FString& PackagePath, const FString& AssetName)
{
	FPonyCreateResult Result;
	CreateInstanceInternal(MasterPath, PackagePath, AssetName, Result);
	return WriteJson(SerializeCreateResult(Result));
}

FString UPonyEditMaterialLibrary::CreateInstanceForSelected(const FString& MasterPath, const FString& PackagePath)
{
	FString MeshName;
	if (!FindSelectedMeshName(MeshName))
	{
		return ErrorJson(TEXT("No selected Actor, or selected Actor has no StaticMesh"));
	}

	if (MeshName.StartsWith(TEXT("SM_")))
	{
		MeshName.RightChopInline(3);
	}

	FString MasterName = FPackageName::GetShortName(MasterPath);
	if (MasterName.StartsWith(GMasterPrefix))
	{
		MasterName.RightChopInline(FCString::Strlen(GMasterPrefix));
	}

	const FString AssetName = FString::Printf(TEXT("MI_%s_%s"), *MeshName, *MasterName);

	FPonyCreateResult CreateResult;
	if (!CreateInstanceInternal(MasterPath, PackagePath, AssetName, CreateResult))
	{
		return WriteJson(SerializeCreateResult(CreateResult));
	}

	const FPonyApplyResult ApplyResult = ApplyMaterialToSelection(CreateResult.Instance, -1);

	const TSharedRef<FJsonObject> Root = SerializeCreateResult(CreateResult);
	Root->SetStringField(TEXT("meshName"), MeshName);
	Root->SetNumberField(TEXT("appliedActors"), ApplyResult.ActorCount);
	Root->SetNumberField(TEXT("appliedComponents"), ApplyResult.ComponentCount);
	Root->SetNumberField(TEXT("appliedSlots"), ApplyResult.SlotCount);
	Root->SetBoolField(TEXT("ok"), ApplyResult.SlotCount > 0);
	if (ApplyResult.SlotCount == 0)
	{
		Root->SetStringField(TEXT("error"), TEXT("MI already created, but no render mesh slot available"));
	}
	return WriteJson(Root);
}

FString UPonyEditMaterialLibrary::ApplyToSelected(const FString& MaterialPath, int32 SlotIndex)
{
	UMaterialInterface* Material = Cast<UMaterialInterface>(LoadAssetObject(MaterialPath));
	if (!Material)
	{
		return ErrorJson(FString::Printf(TEXT("Material does not exist: %s"), *MaterialPath));
	}

	const FPonyApplyResult Result = ApplyMaterialToSelection(Material, SlotIndex);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("ok"), Result.SlotCount > 0);
	Root->SetStringField(TEXT("materialPath"), Material->GetPathName());
	Root->SetNumberField(TEXT("slotIndex"), SlotIndex);
	Root->SetNumberField(TEXT("actors"), Result.ActorCount);
	Root->SetNumberField(TEXT("components"), Result.ComponentCount);
	Root->SetNumberField(TEXT("slots"), Result.SlotCount);
	if (Result.SlotCount == 0)
	{
		Root->SetStringField(TEXT("error"), TEXT("No selected renderable mesh, or slot does not exist"));
	}
	return WriteJson(Root);
}

FString UPonyEditMaterialLibrary::GetInstanceInfo(const FString& MaterialPath)
{
	UObject* Object = LoadAssetObject(MaterialPath);
	UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Object);
	if (!Instance)
	{
		if (UMaterial* Master = Cast<UMaterial>(Object))
		{
			const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetBoolField(TEXT("ok"), true);
			Root->SetBoolField(TEXT("isMaster"), true);
			Root->SetStringField(TEXT("path"), Master->GetPathName());
			return WriteJson(Root);
		}
		return ErrorJson(FString::Printf(TEXT("MI does not exist: %s"), *MaterialPath));
	}

	UMaterialInterface* Parent = Instance->Parent.Get();
	const FString ParentName = Parent ? Parent->GetName() : FString();
	const FPonyMasterSpec* Spec = FindSpec(FName(*ParentName));

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("ok"), true);
	Root->SetBoolField(TEXT("isMaster"), false);
	Root->SetStringField(TEXT("path"), Instance->GetPathName());
	Root->SetStringField(TEXT("parent"), Parent ? Parent->GetPathName() : FString());
	Root->SetBoolField(TEXT("knownContract"), Spec != nullptr);

	TArray<FString> Missing;
	TArray<TSharedPtr<FJsonValue>> ValueArray;

	if (Spec)
	{
		for (const FPonyParamSpec& Param : Spec->Params)
		{
			const TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetStringField(TEXT("name"), Param.Name.ToString());
			Entry->SetStringField(TEXT("type"), ParamTypeToString(Param.Type));

			switch (Param.Type)
			{
			case EPonyParamType::Texture:
			{
				UTexture* Texture = nullptr;
				const bool bFound = Instance->GetTextureParameterValue(
					FHashedMaterialParameterInfo(Param.Name), Texture, false);
				Entry->SetBoolField(TEXT("found"), bFound);
				Entry->SetStringField(TEXT("value"), Texture ? Texture->GetPathName() : FString());
				if (!bFound)
				{
					Missing.Add(Param.Name.ToString());
				}
				break;
			}
			case EPonyParamType::Vector:
			{
				FLinearColor Color = FLinearColor::White;
				const bool bFound = Instance->GetVectorParameterValue(
					FHashedMaterialParameterInfo(Param.Name), Color, false);
				Entry->SetBoolField(TEXT("found"), bFound);
				TArray<TSharedPtr<FJsonValue>> Components;
				Components.Add(MakeShared<FJsonValueNumber>(Color.R));
				Components.Add(MakeShared<FJsonValueNumber>(Color.G));
				Components.Add(MakeShared<FJsonValueNumber>(Color.B));
				Components.Add(MakeShared<FJsonValueNumber>(Color.A));
				Entry->SetArrayField(TEXT("value"), Components);
				if (!bFound)
				{
					Missing.Add(Param.Name.ToString());
				}
				break;
			}
			case EPonyParamType::Scalar:
			{
				float Value = 0.f;
				const bool bFound = Instance->GetScalarParameterValue(
					FHashedMaterialParameterInfo(Param.Name), Value, false);
				Entry->SetBoolField(TEXT("found"), bFound);
				Entry->SetNumberField(TEXT("value"), Value);
				if (!bFound)
				{
					Missing.Add(Param.Name.ToString());
				}
				break;
			}
			case EPonyParamType::StaticSwitch:
			{
				bool bValue = false;
				FGuid Guid;
				const bool bFound = Instance->GetStaticSwitchParameterValue(
					FHashedMaterialParameterInfo(Param.Name), bValue, Guid, false);
				Entry->SetBoolField(TEXT("found"), bFound);
				Entry->SetBoolField(TEXT("value"), bValue);
				if (!bFound)
				{
					Missing.Add(Param.Name.ToString());
				}
				break;
			}
			default:
				break;
			}

			ValueArray.Add(MakeShared<FJsonValueObject>(Entry));
		}
		SetStringArrayField(Root, TEXT("missing"), Missing);
	}

	Root->SetNumberField(TEXT("paramCount"), ValueArray.Num());
	Root->SetArrayField(TEXT("params"), ValueArray);
	return WriteJson(Root);
}
