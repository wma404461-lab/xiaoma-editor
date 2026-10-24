#include "SPonyEditPanel.h"

#include "PonyMeshOps.h"
#include "PonyEditImportLibrary.h"
#include "PonyEditMaterialLibrary.h"
#include "PonyEditScatterLibrary.h"
#include "Misc/Paths.h"

#include "Dom/JsonObject.h"
#include "Input/Reply.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString GetJsonString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		FString Value;
		return Object.IsValid() && Object->TryGetStringField(Field, Value) ? Value : FString();
	}

	int32 GetJsonInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		double Value = 0.0;
		return Object.IsValid() && Object->TryGetNumberField(Field, Value) ? static_cast<int32>(Value) : 0;
	}

	bool GetJsonBool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		bool bValue = false;
		return Object.IsValid() && Object->TryGetBoolField(Field, bValue) ? bValue : false;
	}

	void GetJsonStringArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FString>& OutValues)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values)
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			if (Value.IsValid())
			{
				OutValues.Add(Value->AsString());
			}
		}
	}

	const TCHAR* GModuleNames[8] =
	{
		TEXT("Import"),
		TEXT("Name"),
		TEXT("Material"),
		TEXT("Organize"),
		TEXT("Scatter"),
		TEXT("Combine"),
		TEXT("Terrain"),
		TEXT("Road")
	};

	const TCHAR* GModuleTitles[8] =
	{
		TEXT("Asset Import"),
		TEXT("Asset and Actor Naming"),
		TEXT("Material Library"),
		TEXT("Pivot and Ground Alignment"),
		TEXT("Surface Scatter"),
		TEXT("Prop Assemblies"),
		TEXT("Terrain Shaping"),
		TEXT("Road Creation")
	};

	const TCHAR* GModuleNotes[8] =
	{
		TEXT("Drag in FBX / Texture, name according to rules, archive, assign material, enable collision. M1 connected."),
		TEXT("Asset and Actor Tag Inspection, batch renaming, reference protection, rollback. M1 connected."),
		TEXT("8 standard parent materials + parameter contract + MI generation connected; thumbnail, variants, migrator to next version."),
		TEXT("Instance pivot and asset-level pivot alignment are available; baked scaling, collision reconstruction pending."),
		TEXT("Scatter vegetation/props on selected surface: slope, clustering, random size/rotation, align to ground. M3 v1; viewport brush next version."),
		TEXT("Main item + sub-item set placement, random size, rotation, relative position. M3 connected."),
		TEXT("Draw line + cross-section directly writes landscape height map. M4 connected."),
		TEXT("Draw line to generate terrain shaping, road, materials, and roadside features. M4 connected.")
	};
}

void SPonyEditPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(0.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(10.f, 8.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(6.f, 2.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("P")))
						.Font(FAppStyle::GetFontStyle("HeadingSmall"))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.72f, 1.0f)))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(8.f, 0.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Xiaoma Editor")))
					.Font(FAppStyle::GetFontStyle("HeadingSmall"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				.Padding(FMargin(8.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PonyEdit  v0.4 · M3")))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.60f, 0.66f)))
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					MakeRail()
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(FMargin(10.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
						[
							SAssignNew(ModuleTitleText, STextBlock)
							.Text(GetModuleTitle(CurrentModule))
							.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SAssignNew(Switcher, SWidgetSwitcher)
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(FMargin(10.f, 5.f))
				[
					SAssignNew(StatusText, STextBlock)
					.Text(FText::FromString(TEXT("Ready. After selecting an actor, it can be aligned to ground.")))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.60f, 0.66f)))
					.AutoWrapText(true)
				]
			]
		]
	];

	if (Switcher.IsValid())
	{
		for (int32 Index = 0; Index < 8; ++Index)
		{
			Switcher->AddSlot()[ MakeModulePage(Index) ];
		}
		Switcher->SetActiveWidgetIndex(CurrentModule);
	}
}

TSharedRef<SWidget> SPonyEditPanel::MakeRail()
{
	TSharedRef<SVerticalBox> Rail = SNew(SVerticalBox);

	for (int32 Index = 0; Index < 8; ++Index)
	{
		Rail->AddSlot()
		.AutoHeight()
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(10.f, 6.f))
			.ButtonColorAndOpacity_Lambda([this, Index]()
			{
				return CurrentModule == Index
					? FSlateColor(FLinearColor(0.16f, 0.25f, 0.38f))
					: FSlateColor(FLinearColor(0.08f, 0.09f, 0.10f));
			})
			.OnClicked_Lambda([this, Index]()
			{
				return OnModuleSelected(Index);
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%02d"), Index + 1)))
					.ColorAndOpacity_Lambda([this, Index]()
					{
						return CurrentModule == Index
							? FSlateColor(FLinearColor(0.55f, 0.78f, 1.0f))
							: FSlateColor(FLinearColor(0.38f, 0.42f, 0.47f));
					})
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(GModuleNames[Index]))
					.ColorAndOpacity_Lambda([this, Index]()
					{
						return CurrentModule == Index
							? FSlateColor(FLinearColor(0.80f, 0.88f, 1.0f))
							: FSlateColor(FLinearColor(0.58f, 0.62f, 0.67f));
					})
				]
			]
		];
	}

	Rail->AddSlot()
	.AutoHeight()
	.Padding(FMargin(6.f, 12.f, 6.f, 0.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("M3")))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.50f, 0.56f)))
	];

	return Rail;
}

TSharedRef<SWidget> SPonyEditPanel::MakeModulePage(int32 ModuleIndex)
{
	if (ModuleIndex == 0)
	{
		return MakeImportPage();
	}

	if (ModuleIndex == 1)
	{
		return MakeNamingPage();
	}

	if (ModuleIndex == 2)
	{
		return MakeMaterialPage();
	}

	if (ModuleIndex == 4)
	{
		return MakeScatterPage();
	}

	if (ModuleIndex == 3)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 2.f, 0.f, 8.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("After selecting an actor, align the base face to Z=0 using the bounding box; or permanently move the pivot of used mesh assets to the center of the ground plane.")))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 2.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(12.f, 7.f))
				.OnClicked(this, &SPonyEditPanel::OnSnapToGround)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Instance align to ground (only moves actor)")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 2.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(12.f, 7.f))
				.OnClicked(this, &SPonyEditPanel::OnPivotToBottom)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Asset pivot align to ground (modifies mesh + simple collision)")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Asset-level changes affect all references; if you only want to fix this one, use 'Instance Align to Ground'. Both operations are grouped into a single undo transaction.")))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.55f, 0.60f)))
				.AutoWrapText(true)
			];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f, 0.f, 8.f))
		[
			SNew(STextBlock)
			.Text(GetModuleNote(ModuleIndex))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Position is already set, connect according to the route map.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.52f, 0.57f)))
		];
}

FText SPonyEditPanel::GetModuleTitle(int32 ModuleIndex) const
{
	return FText::FromString(GModuleTitles[FMath::Clamp(ModuleIndex, 0, 7)]);
}

FText SPonyEditPanel::GetModuleNote(int32 ModuleIndex) const
{
	return FText::FromString(GModuleNotes[FMath::Clamp(ModuleIndex, 0, 7)]);
}

TSharedRef<SWidget> SPonyEditPanel::MakeImportPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f, 0.f, 8.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Import png from TestInput/textures: automatically adds T_ prefix, sets normals/linear according to _N/_R, saves immediately after import. Drag and listen functionality connected to M2.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(12.f, 8.f))
			.OnClicked(this, &SPonyEditPanel::OnImportTextures)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Import TestInput/textures")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Target: /Game/PonyEditTest/Textures")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.49f, 0.53f)))
		];
}

FReply SPonyEditPanel::OnImportTextures()
{
	const FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("TestInput/textures"));
	const FString Result = UPonyEditImportLibrary::ImportTextures(SourceDir, TEXT("/Game/PonyEditTest/Textures"));

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Result));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SPonyEditPanel::MakeMaterialPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f, 0.f, 8.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("8 standard parent materials, sharing a common parameter contract. Select a parent material, create a MI using the mesh name of the selected actor and apply it directly; the parent material itself can also be applied directly.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 6.f))
				.OnClicked(this, &SPonyEditPanel::OnRefreshMaterials)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Refresh material library")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 6.f))
				.OnClicked(this, &SPonyEditPanel::OnCreateMaterialInstance)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Create MI and apply to selected")))
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(10.f, 6.f))
			.OnClicked(this, &SPonyEditPanel::OnApplyMasterMaterial)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Parent material directly applied to selected")))
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(MaterialRackBox, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[
			SAssignNew(MaterialDetailText, STextBlock)
			.Text(FText::FromString(TEXT("Click 'Refresh Material Library' to view the 8 parent materials; click a row to select it.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.55f, 0.60f)))
			.AutoWrapText(true)
		];
}

FReply SPonyEditPanel::OnRefreshMaterials()
{
	RebuildMaterialRack();

	int32 Healthy = 0;
	int32 Existing = 0;
	int32 Instances = 0;
	for (const FPonyMaterialRackEntry& Entry : MaterialRack)
	{
		if (Entry.bOk)
		{
			++Healthy;
		}
		if (Entry.bExists)
		{
			++Existing;
		}
		Instances += Entry.MICount;
	}

	if (StatusText.IsValid() && MaterialRack.Num() > 0)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Material Asset: %d/%d parent material contracts complete (%d exist), total %d MI"),
			Healthy, MaterialRack.Num(), Existing, Instances)));
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnSelectMaterialMaster(int32 Index)
{
	if (MaterialRack.IsValidIndex(Index))
	{
		SelectedMaterialIndex = Index;
		UpdateMaterialDetail();

		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(FString::Printf(
				TEXT("Selected %s; can create MI or directly apply to selected Actor."), *MaterialRack[Index].Name)));
		}
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnCreateMaterialInstance()
{
	const FString MasterPath = GetSelectedMasterPath();
	if (MasterPath.IsEmpty())
	{
		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(TEXT("First select a parent material in the material asset browser.")));
		}
		return FReply::Handled();
	}

	const FString Json = UPonyEditMaterialLibrary::CreateInstanceForSelected(
		MasterPath, TEXT("/Game/PonyEdit/Materials/Instances"));

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	const bool bParsed = FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid();
	const bool bOk = bParsed && GetJsonBool(Root, TEXT("ok"));

	if (StatusText.IsValid())
	{
		if (bOk)
		{
			StatusText->SetText(FText::FromString(FString::Printf(
				TEXT("Created %s and applied to %d slots."),
				*GetJsonString(Root, TEXT("assetName")),
				GetJsonInt(Root, TEXT("appliedSlots")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("Return result is not valid JSON");
			StatusText->SetText(FText::FromString(TEXT("Failed to create MI:") + Error));
		}
	}

	if (bOk)
	{
		RebuildMaterialRack();
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnApplyMasterMaterial()
{
	const FString MasterPath = GetSelectedMasterPath();
	if (MasterPath.IsEmpty())
	{
		if (StatusText.IsValid())
		{
			StatusText->SetText(FText::FromString(TEXT("First select a parent material in the material asset browser.")));
		}
		return FReply::Handled();
	}

	const FString Json = UPonyEditMaterialLibrary::ApplyToSelected(MasterPath, -1);

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	const bool bParsed = FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid();
	const bool bOk = bParsed && GetJsonBool(Root, TEXT("ok"));

	if (StatusText.IsValid())
	{
		if (bOk)
		{
			StatusText->SetText(FText::FromString(FString::Printf(
				TEXT("Applied to %d Actors, %d slots."),
				GetJsonInt(Root, TEXT("actors")),
				GetJsonInt(Root, TEXT("slots")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("Return result is not valid JSON");
			StatusText->SetText(FText::FromString(TEXT("Failed to apply material:") + Error));
		}
	}
	return FReply::Handled();
}

FString SPonyEditPanel::GetSelectedMasterPath() const
{
	return MaterialRack.IsValidIndex(SelectedMaterialIndex)
		? MaterialRack[SelectedMaterialIndex].Path
		: FString();
}

void SPonyEditPanel::RebuildMaterialRack()
{
	if (!MaterialRackBox.IsValid())
	{
		return;
	}

	MaterialRackBox->ClearChildren();
	MaterialRack.Reset();

	const FString Json = UPonyEditMaterialLibrary::GetRack();
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		MaterialRackBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Failed to parse material asset data.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.55f, 0.50f)))
		];
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* Masters = nullptr;
	if (Root->TryGetArrayField(TEXT("masters"), Masters) && Masters)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Masters)
		{
			const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
			if (!Object.IsValid())
			{
				continue;
			}

			FPonyMaterialRackEntry Entry;
			Entry.Name = GetJsonString(Object, TEXT("name"));
			Entry.Usage = GetJsonString(Object, TEXT("usage"));
			Entry.Path = GetJsonString(Object, TEXT("path"));
			Entry.ParamCount = GetJsonInt(Object, TEXT("paramCount"));
			Entry.ExpectedCount = GetJsonInt(Object, TEXT("expectedCount"));
			Entry.MICount = GetJsonInt(Object, TEXT("miCount"));
			Entry.bExists = GetJsonBool(Object, TEXT("exists"));
			Entry.bOk = GetJsonBool(Object, TEXT("ok"));
			GetJsonStringArray(Object, TEXT("missing"), Entry.Missing);
			MaterialRack.Add(Entry);
		}
	}

	if (MaterialRack.Num() == 0)
	{
		MaterialRackBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No parent materials found. Run Scripts/make_masters.py to generate 8 parent materials.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.70f, 0.45f)))
			.AutoWrapText(true)
		];
		return;
	}

	SelectedMaterialIndex = FMath::Clamp(SelectedMaterialIndex, 0, MaterialRack.Num() - 1);

	for (int32 Index = 0; Index < MaterialRack.Num(); ++Index)
	{
		const FPonyMaterialRackEntry& Entry = MaterialRack[Index];
		const FLinearColor StatusColor = Entry.bOk
			? FLinearColor(0.45f, 0.78f, 0.52f)
			: (Entry.bExists ? FLinearColor(0.95f, 0.55f, 0.50f) : FLinearColor(0.52f, 0.55f, 0.58f));

		FString Detail = FString::Printf(TEXT("%d/%d parameters · %d MI"),
			Entry.ParamCount, Entry.ExpectedCount, Entry.MICount);
		if (Entry.Missing.Num() > 0)
		{
			Detail += TEXT(" · Missing ") + FString::Join(Entry.Missing, TEXT(", "));
		}
		else if (!Entry.bExists)
		{
			Detail += TEXT(" · Asset does not exist");
		}

		MaterialRackBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 3.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Fill)
			.ContentPadding(FMargin(8.f, 5.f))
			.ButtonColorAndOpacity_Lambda([this, Index]()
			{
				return SelectedMaterialIndex == Index
					? FSlateColor(FLinearColor(0.16f, 0.25f, 0.38f))
					: FSlateColor(FLinearColor(0.09f, 0.10f, 0.12f));
			})
			.OnClicked_Lambda([this, Index]()
			{
				return OnSelectMaterialMaster(Index);
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("●")))
					.ColorAndOpacity(FSlateColor(StatusColor))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%s  ·  %s"), *Entry.Name, *Entry.Usage)))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.89f, 0.93f)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(Detail))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.55f, 0.60f)))
						.AutoWrapText(true)
					]
				]
			]
		];
	}

	UpdateMaterialDetail();
}

void SPonyEditPanel::UpdateMaterialDetail()
{
	if (!MaterialDetailText.IsValid())
	{
		return;
	}

	if (!MaterialRack.IsValidIndex(SelectedMaterialIndex))
	{
		MaterialDetailText->SetText(FText::FromString(TEXT("Click 'Refresh Material Library' to view the 8 parent materials; click a row to select it.")));
		return;
	}

	const FPonyMaterialRackEntry& Entry = MaterialRack[SelectedMaterialIndex];
	FString Text = FString::Printf(TEXT("%s\n%s · Parameter %d/%d · MI %d"),
		*Entry.Path, *Entry.Usage, Entry.ParamCount, Entry.ExpectedCount, Entry.MICount);

	if (Entry.Missing.Num() > 0)
	{
		Text += TEXT("\nMissing parameter:") + FString::Join(Entry.Missing, TEXT(", "));
	}
	else if (!Entry.bExists)
	{
		Text += TEXT("\nAsset does not exist: run Scripts/make_masters.py first.");
	}
	else
	{
		Text += TEXT("\nContract complete.");
	}

	MaterialDetailText->SetText(FText::FromString(Text));
}

TSharedRef<SWidget> SPonyEditPanel::MakeScatterPage()
{
	auto MakeNumberBox = [](const FString& Default, TSharedPtr<SEditableTextBox>& OutBox) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(72.f)
			[
				SAssignNew(OutBox, SEditableTextBox)
				.Text(FText::FromString(Default))
			];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("M3 v1: Select ground/grid to scatter a patch. Cast downward ray to find surface, supports slope filtering, clustering, random size/rotation, size-based alignment to ground. Viewport brush and HISM batch will be implemented in next version.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(64.f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Grid")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SAssignNew(ScatterMeshBox, SEditableTextBox)
				.Text(FText::FromString(TEXT("/Engine/BasicShapes/Cone.Cone")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(64.f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Count")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("40"), ScatterCountBox) ]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(20.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Radius")))
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("500"), ScatterRadiusBox) ]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SBox)
				.WidthOverride(64.f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Scale")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("0.6"), ScatterMinScaleBox) ]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(6.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("–")))
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("1.4"), ScatterMaxScaleBox) ]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(20.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Slope°")))
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("35"), ScatterSlopeBox) ]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(ScatterHismBox, SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("HISM Batch")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Instance multiple: when checked, one Actor is created per area, using instanced components for batching.")))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.52f, 0.57f)))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 7.f))
				.OnClicked(this, &SPonyEditPanel::OnScatterSurface)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Plant a patch on the selected surface")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 7.f))
				.OnClicked(this, &SPonyEditPanel::OnClearScatter)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Clear planting")))
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Default: 4 clusters, cluster radius 120. Align to ground based on the mesh bounding box minZ. Clear only removes Actors with the PonyScatter tag.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.48f, 0.52f, 0.57f)))
			.AutoWrapText(true)
		];
}

FReply SPonyEditPanel::OnScatterSurface()
{
	auto ReadInt = [](const TSharedPtr<SEditableTextBox>& Box, int32 Default)
	{
		if (!Box.IsValid() || Box->GetText().IsEmpty())
		{
			return Default;
		}
		return FCString::Atoi(*Box->GetText().ToString());
	};
	auto ReadFloat = [](const TSharedPtr<SEditableTextBox>& Box, float Default)
	{
		if (!Box.IsValid() || Box->GetText().IsEmpty())
		{
			return Default;
		}
		return FCString::Atof(*Box->GetText().ToString());
	};

	TSharedRef<FJsonObject> Params = MakeShared<FJsonObject>();
	Params->SetStringField(TEXT("meshPath"),
		ScatterMeshBox.IsValid() ? ScatterMeshBox->GetText().ToString() : TEXT("/Engine/BasicShapes/Cone.Cone"));
	Params->SetNumberField(TEXT("count"), ReadInt(ScatterCountBox, 40));
	Params->SetNumberField(TEXT("radius"), ReadFloat(ScatterRadiusBox, 500.f));
	Params->SetNumberField(TEXT("minScale"), ReadFloat(ScatterMinScaleBox, 0.6f));
	Params->SetNumberField(TEXT("maxScale"), ReadFloat(ScatterMaxScaleBox, 1.4f));
	Params->SetNumberField(TEXT("maxSlopeDegrees"), ReadFloat(ScatterSlopeBox, 35.f));
	Params->SetStringField(TEXT("spawnMode"),
		ScatterHismBox.IsValid() && ScatterHismBox->IsChecked() ? TEXT("hism") : TEXT("actor"));
	Params->SetNumberField(TEXT("clusterCount"), 4);
	Params->SetNumberField(TEXT("clusterRadius"), 120);
	Params->SetBoolField(TEXT("alignToNormal"), true);
	Params->SetBoolField(TEXT("dropToBase"), true);

	FString ParamsJson;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ParamsJson);
	FJsonSerializer::Serialize(Params, Writer);

	const FString ResultJson = UPonyEditScatterLibrary::ScatterOnSelection(ParamsJson);

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultJson);
	const bool bParsed = FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid();
	const bool bOk = bParsed && GetJsonBool(Root, TEXT("ok"));

	if (StatusText.IsValid())
	{
		if (bOk)
		{
			StatusText->SetText(FText::FromString(FString::Printf(
				TEXT("Planting: placed %d / %d, slope skipped %d, missed %d"),
				GetJsonInt(Root, TEXT("placed")),
				GetJsonInt(Root, TEXT("requested")),
				GetJsonInt(Root, TEXT("skippedSlope")),
				GetJsonInt(Root, TEXT("skippedNoHit")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("Return result is not valid JSON");
			StatusText->SetText(FText::FromString(TEXT("Planting failed:") + Error));
		}
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnClearScatter()
{
	const FString ResultJson = UPonyEditScatterLibrary::ClearScatter();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultJson);
	const bool bParsed = FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid();
	const bool bOk = bParsed && GetJsonBool(Root, TEXT("ok"));

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(bOk
			? FString::Printf(TEXT("Cleared %d planting instances"), GetJsonInt(Root, TEXT("removed")))
			: TEXT("Failed to clear planting")));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SPonyEditPanel::MakeNamingPage()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f, 0.f, 8.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Scan /Game: check prefix, spaces/Chinese characters, PascalCase, sequence zero-padding, _final/_copy remnants; rename while preserving references.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.66f, 0.70f)))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 6.f))
				.OnClicked(this, &SPonyEditPanel::OnScanNaming)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Scan /Game")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(10.f, 6.f))
				.OnClicked(this, &SPonyEditPanel::OnFixNaming)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Fix all")))
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(10.f, 6.f))
			.OnClicked(this, &SPonyEditPanel::OnFixActorLabels)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Fix selected Actor tags (ENV_...)")))
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(NamingListBox, SVerticalBox)
			]
		];
}

FReply SPonyEditPanel::OnScanNaming()
{
	NamingIssues.Reset();

	int32 Scanned = 0;
	FPonyNamingService::ScanPath(TEXT("/Game"), NamingIssues, Scanned);
	RebuildNamingList();

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Scanned %d assets, %d naming issues"), Scanned, NamingIssues.Num())));
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnFixNaming()
{
	if (NamingIssues.Num() == 0)
	{
		OnScanNaming();
	}

	TArray<FString> Renamed;
	TArray<FString> Failed;
	const int32 RenamedCount = FPonyNamingService::ApplyFixes(NamingIssues, Renamed, Failed);

	NamingIssues.Reset();
	int32 Scanned = 0;
	FPonyNamingService::ScanPath(TEXT("/Game"), NamingIssues, Scanned);
	RebuildNamingList();

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Renamed %d, failed %d, %d issues remaining"), RenamedCount, Failed.Num(), NamingIssues.Num())));
	}
	return FReply::Handled();
}

FReply SPonyEditPanel::OnFixActorLabels()
{
	TArray<FString> Renamed;
	const int32 Changed = FPonyNamingService::FixSelectedActorLabels(Renamed);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Actor tags: fixed %d (only applies to selected Actors)"), Changed)));
	}
	return FReply::Handled();
}

void SPonyEditPanel::RebuildNamingList(){
	if (!NamingListBox.IsValid())
	{
		return;
	}

	NamingListBox->ClearChildren();

	if (NamingIssues.Num() == 0)
	{
		NamingListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No issues found.")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.68f, 0.52f)))
		];
		return;
	}

	const int32 MaxRows = 24;
	for (int32 Index = 0; Index < FMath::Min(MaxRows, NamingIssues.Num()); ++Index)
	{
		const FPonyNamingIssue& Issue = NamingIssues[Index];
		const FLinearColor NameColor = Issue.Severity >= 2
			? FLinearColor(0.95f, 0.65f, 0.55f)
			: FLinearColor(0.60f, 0.70f, 0.82f);

		NamingListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 4.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("●  %s  →  %s"), *Issue.CurrentName, *Issue.SuggestedName)))
				.ColorAndOpacity(FSlateColor(NameColor))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Issue.Reason))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.49f, 0.53f)))
			]
		];
	}

	if (NamingIssues.Num() > MaxRows)
	{
		NamingListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d more items…"), NamingIssues.Num() - MaxRows)))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.49f, 0.53f)))
		];
	}
}

FReply SPonyEditPanel::OnModuleSelected(int32 ModuleIndex)
{
	CurrentModule = FMath::Clamp(ModuleIndex, 0, 7);

	if (Switcher.IsValid())
	{
		Switcher->SetActiveWidgetIndex(CurrentModule);
	}
	if (ModuleTitleText.IsValid())
	{
		ModuleTitleText->SetText(GetModuleTitle(CurrentModule));
	}
	if (StatusText.IsValid())
	{
		if (CurrentModule == 2)
		{
			StatusText->SetText(FText::FromString(TEXT("Materials: select a parent material, then create an instance or apply it to selected Actors.")));
		}
		else if (CurrentModule == 3)
		{
			StatusText->SetText(FText::FromString(TEXT("Ready. After selecting an actor, it can be aligned to ground.")));
		}
		else if (CurrentModule == 4)
		{
			StatusText->SetText(FText::FromString(TEXT("Scatter v1: select a surface or mesh, then click Scatter. Clear removes only PonyScatter objects.")));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("This module is planned for milestones M1–M4. Its panel is reserved.")));
		}
	}

	if (CurrentModule == 2 && MaterialRack.Num() == 0)
	{
		OnRefreshMaterials();
	}

	return FReply::Handled();
}

FReply SPonyEditPanel::OnSnapToGround()
{
	FString Message;
	FPonyMeshOps::SnapSelectedActorsToGround(Message);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Message));
	}

	return FReply::Handled();
}

FReply SPonyEditPanel::OnPivotToBottom()
{
	FString Message;
	FPonyMeshOps::PivotMeshesToBottom(Message);

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Message));
	}

	return FReply::Handled();
}

