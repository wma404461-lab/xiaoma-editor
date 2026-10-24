#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "PonyNamingService.h"

class SWidgetSwitcher;
class STextBlock;
class SVerticalBox;
class SEditableTextBox;
class SCheckBox;

/** 材质架的一行，来自 PonyEditMaterialLibrary.GetRack()。 */
struct FPonyMaterialRackEntry
{
	FString Name;
	FString Usage;
	FString Path;
	int32 ParamCount = 0;
	int32 ExpectedCount = 0;
	int32 MICount = 0;
	bool bExists = false;
	bool bOk = false;
	TArray<FString> Missing;
};

/**
 * 小马编辑主面板（M3）：
 * 左侧 8 个模块，已接「导入」「命名」「材质」「整理」「种植」。
 */
class SPonyEditPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPonyEditPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SelectModule(int32 Index) { OnModuleSelected(Index); }
	void RunNamingScan() { OnScanNaming(); }
	void RunImport() { OnImportTextures(); }
	void RunMaterialRack() { OnRefreshMaterials(); }

private:
	TSharedRef<SWidget> MakeRail();
	TSharedRef<SWidget> MakeModulePage(int32 ModuleIndex);
	TSharedRef<SWidget> MakeNamingPage();
	TSharedRef<SWidget> MakeImportPage();
	TSharedRef<SWidget> MakeMaterialPage();
	TSharedRef<SWidget> MakeScatterPage();
	FText GetModuleTitle(int32 ModuleIndex) const;
	FText GetModuleNote(int32 ModuleIndex) const;
	void RebuildNamingList();
	void RebuildMaterialRack();
	void UpdateMaterialDetail();
	FString GetSelectedMasterPath() const;

	FReply OnModuleSelected(int32 ModuleIndex);
	FReply OnSnapToGround();
	FReply OnPivotToBottom();
	FReply OnScanNaming();
	FReply OnFixNaming();
	FReply OnFixActorLabels();
	FReply OnImportTextures();
	FReply OnRefreshMaterials();
	FReply OnSelectMaterialMaster(int32 Index);
	FReply OnCreateMaterialInstance();
	FReply OnApplyMasterMaterial();
	FReply OnScatterSurface();
	FReply OnClearScatter();

	TSharedPtr<SWidgetSwitcher> Switcher;
	TSharedPtr<STextBlock> ModuleTitleText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SVerticalBox> NamingListBox;
	TSharedPtr<SVerticalBox> MaterialRackBox;
	TSharedPtr<STextBlock> MaterialDetailText;
	TSharedPtr<SEditableTextBox> ScatterMeshBox;
	TSharedPtr<SEditableTextBox> ScatterCountBox;
	TSharedPtr<SEditableTextBox> ScatterRadiusBox;
	TSharedPtr<SEditableTextBox> ScatterMinScaleBox;
	TSharedPtr<SEditableTextBox> ScatterMaxScaleBox;
	TSharedPtr<SEditableTextBox> ScatterSlopeBox;
	TSharedPtr<SCheckBox> ScatterHismBox;

	TArray<FPonyNamingIssue> NamingIssues;
	TArray<FPonyMaterialRackEntry> MaterialRack;
	int32 CurrentModule = 3;
	int32 SelectedMaterialIndex = 0;
};
