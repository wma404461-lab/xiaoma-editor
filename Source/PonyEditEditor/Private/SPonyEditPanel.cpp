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
		TEXT("导入"),
		TEXT("命名"),
		TEXT("材质"),
		TEXT("整理"),
		TEXT("种植"),
		TEXT("组合"),
		TEXT("地形"),
		TEXT("道路")
	};

	const TCHAR* GModuleTitles[8] =
	{
		TEXT("快车道 · 资产导入"),
		TEXT("命名警察 · 资产与 Actor 命名"),
		TEXT("材质架 · 标准母材质"),
		TEXT("整理台 · 枢轴归底"),
		TEXT("种草机 · 表面种植"),
		TEXT("搭配师 · 装饰组合"),
		TEXT("地形笔 · 画线起伏"),
		TEXT("铺路工 · 画线成路")
	};

	const TCHAR* GModuleNotes[8] =
	{
		TEXT("拖入 FBX / 贴图，按规则命名、归档、配材质、开碰撞。M1 接。"),
		TEXT("资产与 Actor 标签体检、批量改名、引用保护、回滚。M1 接。"),
		TEXT("8 个标准母材质 + 参数契约 + MI 生成已接；缩略图、变体、迁移器下一版。"),
		TEXT("实例归底和资产级枢轴归底已经可用；烘焙缩放、碰撞重建待补。"),
		TEXT("在选中表面撒植被/道具：坡度、聚簇、随机尺寸/朝向、落底。M3 v1；视口笔刷下一版。"),
		TEXT("主件 + 子件成套摆放，随机尺寸、旋转、相对位置。M3 接。"),
		TEXT("画线 + 剖面直接写 Landscape 高度图。M4 接。"),
		TEXT("画线生成地形整形、路面、材质与路边附属。M4 接。")
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
						.Text(FText::FromString(TEXT("马")))
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
					.Text(FText::FromString(TEXT("小马编辑")))
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
					.Text(FText::FromString(TEXT("就绪。选中 Actor 后可以归底。")))
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
				.Text(FText::FromString(TEXT("选中 Actor 后，按包围盒把底面对到 Z=0；或把用到的网格资产枢轴永久挪到底面中心。")))
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
					.Text(FText::FromString(TEXT("实例归底（只挪 Actor）")))
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
					.Text(FText::FromString(TEXT("资产枢轴归底（改网格 + 简单碰撞）")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("资产级会改所有引用；只想修当前这一个就用「实例归底」。两种操作都进一个撤销事务。")))
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
			.Text(FText::FromString(TEXT("位置已留好，按路线图接进来。")))
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
			.Text(FText::FromString(TEXT("从 TestInput/textures 导入 png：自动加 T_ 前缀、按 _N/_R 设法线/线性、导入后立即保存。拖拽和监听在 M2 接。")))
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
				SNew(STextBlock).Text(FText::FromString(TEXT("导入 TestInput/textures")))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 10.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("目标：/Game/PonyEditTest/Textures")))
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
			.Text(FText::FromString(TEXT("8 个标准母材质，共用一套参数契约。选一个母材质，用选中 Actor 的网格名建 MI 并直接套上；母材质本身也可直接套。")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("刷新材质架")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("建 MI 并应用到选中")))
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
				SNew(STextBlock).Text(FText::FromString(TEXT("母材质直接套到选中")))
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
			.Text(FText::FromString(TEXT("点「刷新材质架」查看 8 个母材质；点一行选中。")))
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
			TEXT("材质架：%d/%d 个母材质契约完整（%d 个存在），共 %d 个 MI"),
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
				TEXT("已选中 %s；可以建 MI 或直接套到选中 Actor。"), *MaterialRack[Index].Name)));
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
			StatusText->SetText(FText::FromString(TEXT("先在材质架里选一个母材质。")));
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
				TEXT("已创建 %s，并套到 %d 个槽位。"),
				*GetJsonString(Root, TEXT("assetName")),
				GetJsonInt(Root, TEXT("appliedSlots")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("返回结果不是合法 JSON");
			StatusText->SetText(FText::FromString(TEXT("建 MI 失败：") + Error));
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
			StatusText->SetText(FText::FromString(TEXT("先在材质架里选一个母材质。")));
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
				TEXT("已套到 %d 个 Actor、%d 个槽位。"),
				GetJsonInt(Root, TEXT("actors")),
				GetJsonInt(Root, TEXT("slots")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("返回结果不是合法 JSON");
			StatusText->SetText(FText::FromString(TEXT("套材质失败：") + Error));
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
			.Text(FText::FromString(TEXT("材质架数据解析失败。")))
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
			.Text(FText::FromString(TEXT("没有读到母材质。先运行 Scripts/make_masters.py 生成 8 个母材质。")))
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

		FString Detail = FString::Printf(TEXT("%d/%d 参数 · %d 个 MI"),
			Entry.ParamCount, Entry.ExpectedCount, Entry.MICount);
		if (Entry.Missing.Num() > 0)
		{
			Detail += TEXT(" · 缺 ") + FString::Join(Entry.Missing, TEXT(", "));
		}
		else if (!Entry.bExists)
		{
			Detail += TEXT(" · 资产不存在");
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
		MaterialDetailText->SetText(FText::FromString(TEXT("点「刷新材质架」查看 8 个母材质；点一行选中。")));
		return;
	}

	const FPonyMaterialRackEntry& Entry = MaterialRack[SelectedMaterialIndex];
	FString Text = FString::Printf(TEXT("%s\n%s · 参数 %d/%d · MI %d"),
		*Entry.Path, *Entry.Usage, Entry.ParamCount, Entry.ExpectedCount, Entry.MICount);

	if (Entry.Missing.Num() > 0)
	{
		Text += TEXT("\n缺参数：") + FString::Join(Entry.Missing, TEXT(", "));
	}
	else if (!Entry.bExists)
	{
		Text += TEXT("\n资产不存在：先运行 Scripts/make_masters.py。");
	}
	else
	{
		Text += TEXT("\n契约完整。");
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
			.Text(FText::FromString(TEXT("M3 v1：选中地表 / 网格后撒一片。向下打射线找表面，支持坡度过滤、聚簇、随机尺寸/朝向、按尺寸落底。视口拖拽笔刷和 HISM 合批下一版。")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("网格")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("数量")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()[ MakeNumberBox(TEXT("40"), ScatterCountBox) ]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(20.f, 0.f, 8.f, 0.f))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("半径")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("缩放")))
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
				SNew(STextBlock).Text(FText::FromString(TEXT("坡度°")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("HISM 合批")))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			.Padding(FMargin(10.f, 0.f, 0.f, 0.f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("实例多时勾选：整片只生成一个 Actor，用实例化组件合批。")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("在选中表面种一片")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("清空种植")))
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("默认 4 簇、簇半径 120，落底按网格包围盒 minZ 自动抬高。清空只删带 PonyScatter 标签的 Actor。")))
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
				TEXT("种植：放置 %d / %d，坡度跳过 %d，未命中 %d"),
				GetJsonInt(Root, TEXT("placed")),
				GetJsonInt(Root, TEXT("requested")),
				GetJsonInt(Root, TEXT("skippedSlope")),
				GetJsonInt(Root, TEXT("skippedNoHit")))));
		}
		else
		{
			const FString Error = bParsed
				? GetJsonString(Root, TEXT("error"))
				: TEXT("返回结果不是合法 JSON");
			StatusText->SetText(FText::FromString(TEXT("种植失败：") + Error));
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
			? FString::Printf(TEXT("已清空 %d 个种植实例"), GetJsonInt(Root, TEXT("removed")))
			: TEXT("清空种植失败")));
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
			.Text(FText::FromString(TEXT("扫描 /Game：检查前缀、空格/中文、PascalCase、序号补零、_final/_copy 残留，改名保留引用。")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("扫描 /Game")))
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
					SNew(STextBlock).Text(FText::FromString(TEXT("修复全部")))
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
				SNew(STextBlock).Text(FText::FromString(TEXT("修选中 Actor 标签（ENV_…）")))
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
			TEXT("扫描 %d 个资产，%d 个命名问题"), Scanned, NamingIssues.Num())));
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
			TEXT("改名 %d 个，失败 %d 个，剩余 %d 个问题"), RenamedCount, Failed.Num(), NamingIssues.Num())));
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
			TEXT("Actor 标签：改了 %d 个（选中的 Actor 才会处理）"), Changed)));
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
			.Text(FText::FromString(TEXT("没有发现问题。")))
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
			.Text(FText::FromString(FString::Printf(TEXT("还有 %d 条…"), NamingIssues.Num() - MaxRows)))
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
			StatusText->SetText(FText::FromString(TEXT("材质架：先选母材质，再建 MI 或直接套到选中 Actor。")));
		}
		else if (CurrentModule == 3)
		{
			StatusText->SetText(FText::FromString(TEXT("就绪。选中 Actor 后可以归底。")));
		}
		else if (CurrentModule == 4)
		{
			StatusText->SetText(FText::FromString(TEXT("种植 v1：选中地表 / 网格后点「种一片」；清空只删 PonyScatter。")));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("该模块排在 M1–M4，面板位置已留好。")));
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

