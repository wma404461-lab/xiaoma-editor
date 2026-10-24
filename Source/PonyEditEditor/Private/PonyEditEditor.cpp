#include "PonyEditEditor.h"

#include "PonyMeshOps.h"
#include "PonyEditMaterialLibrary.h"
#include "PonyEditScatterLibrary.h"
#include "PonyEditSettings.h"
#include "SPonyEditPanel.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Editor/EditorEngine.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EditorViewportClient.h"
#include "UnrealClient.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "ToolMenus.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "PonyEdit"

DEFINE_LOG_CATEGORY_STATIC(LogPonyEdit, Log, All);

IMPLEMENT_MODULE(FPonyEditEditorModule, PonyEditEditor)

namespace
{
	void CapturePanelShot()
	{
		TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FPonyEditEditorModule::GetTabName()));
		if (!Tab.IsValid())
		{
			UE_LOG(LogPonyEdit, Warning, TEXT("[PonyEdit] Shot: panel tab is not open."));
			return;
		}

		TArray<FColor> Pixels;
		FIntVector Size = FIntVector::ZeroValue;
		if (!FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size))
		{
			UE_LOG(LogPonyEdit, Warning, TEXT("[PonyEdit] Shot: TakeScreenshot failed."));
			return;
		}

		const FString FilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PonyEdit"), TEXT("panel.png"));
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

		// Slate 截图的 alpha 常为 0，PNG 会显示成全白；强制不透明。
		for (FColor& Pixel : Pixels)
		{
			Pixel.A = 255;
		}

		TArray64<uint8> PngData;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), PngData);
		const bool bSaved = FFileHelper::SaveArrayToFile(PngData, *FilePath);

		int32 NonWhite = 0;
		for (const FColor& Pixel : Pixels)
		{
			if (Pixel.R < 250 || Pixel.G < 250 || Pixel.B < 250)
			{
				++NonWhite;
			}
		}

		UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] Shot: %dx%d saved=%d nonWhite=%d path=%s"),
			Size.X, Size.Y, bSaved ? 1 : 0, NonWhite, *FilePath);
	}

	FAutoConsoleCommand GPonyOpenCommand(
		TEXT("PonyEdit.Open"),
		TEXT("打开小马编辑面板。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
		}));

	FAutoConsoleCommand GPonySnapCommand(
		TEXT("PonyEdit.SnapSelected"),
		TEXT("把选中的 Actor 按包围盒归到 Z=0。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FString Message;
			const int32 Count = FPonyMeshOps::SnapSelectedActorsToGround(Message);
			UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] SnapSelected -> %d, %s"), Count, *Message);
		}));

	FAutoConsoleCommand GPonyPivotCommand(
		TEXT("PonyEdit.PivotSelected"),
		TEXT("对选中 Actor 用到的 StaticMesh 执行资产级枢轴归底。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FString Message;
			const int32 Count = FPonyMeshOps::PivotMeshesToBottom(Message);
			UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] PivotSelected -> %d, %s"), Count, *Message);
		}));

	FAutoConsoleCommand GPonyInfoCommand(
		TEXT("PonyEdit.Info"),
		TEXT("打印小马编辑的加载状态与当前设置。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			const UPonyEditSettings* Settings = GetDefault<UPonyEditSettings>();
			UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] Info: tab=%s, importRoot=%s, safeMode=%d, watch=%d"),
				*FPonyEditEditorModule::GetTabName().ToString(),
				Settings ? *Settings->DefaultImportRoot : TEXT("<none>"),
				Settings && Settings->bSafeMode ? 1 : 0,
				Settings && Settings->bWatchImportFolder ? 1 : 0);
		}));

	FAutoConsoleCommand GPonyShotCommand(
		TEXT("PonyEdit.Shot"),
		TEXT("把当前小马编辑面板截成 PNG：Saved/PonyEdit/panel.png。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			CapturePanelShot();
		}));

	FAutoConsoleCommand GPonySelfTestCommand(
		TEXT("PonyEdit.SelfTest"),
		TEXT("打开面板，等待 4 秒后截图并退出，用于无头自检。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				CapturePanelShot();
				FPlatformMisc::RequestExit(false);
				return false;
			}), 4.0f);
		}));

	FAutoConsoleCommand GPonySelfTestImportCommand(
		TEXT("PonyEdit.SelfTestImport"),
		TEXT("打开导入页、执行一次导入，截图后退出。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FPonyEditEditorModule::GetTabName())))
				{
					TSharedPtr<SPonyEditPanel> Panel = StaticCastSharedRef<SPonyEditPanel>(Tab->GetContent());
					Panel->SelectModule(0);
					Panel->RunImport();
				}
				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
				{
					CapturePanelShot();
					FPlatformMisc::RequestExit(false);
					return false;
				}), 2.0f);
				return false;
			}), 3.0f);
		}));

	FAutoConsoleCommand GPonySelfTestNamingCommand(
		TEXT("PonyEdit.SelfTestNaming"),
		TEXT("打开命名页、扫描 /Game，截图后退出。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FPonyEditEditorModule::GetTabName())))
				{
					TSharedRef<SWidget> Content = Tab->GetContent();
					TSharedPtr<SPonyEditPanel> Panel = StaticCastSharedRef<SPonyEditPanel>(Content);
					Panel->SelectModule(1);
					Panel->RunNamingScan();
				}

				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
				{
					CapturePanelShot();
					FPlatformMisc::RequestExit(false);
					return false;
				}), 2.0f);

				return false;
			}), 3.0f);
		}));

	FAutoConsoleCommand GPonyMaterialRackCommand(
		TEXT("PonyEdit.MaterialRack"),
		TEXT("把材质架 JSON 打到日志。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] MaterialRack: %s"), *UPonyEditMaterialLibrary::GetRack());
		}));

	FAutoConsoleCommand GPonySelfTestMaterialCommand(
		TEXT("PonyEdit.SelfTestMaterial"),
		TEXT("打开材质页、刷新材质架，截图后退出。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FPonyEditEditorModule::GetTabName())))
				{
					TSharedRef<SWidget> Content = Tab->GetContent();
					TSharedPtr<SPonyEditPanel> Panel = StaticCastSharedRef<SPonyEditPanel>(Content);
					Panel->SelectModule(2);
					Panel->RunMaterialRack();
				}

				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
				{
					CapturePanelShot();
					FPlatformMisc::RequestExit(false);
					return false;
				}), 3.0f);

				return false;
			}), 3.0f);
		}));

	FAutoConsoleCommand GPonySelfTestScatterCommand(
		TEXT("PonyEdit.SelfTestScatter"),
		TEXT("打开种植页，截图后退出。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FPonyEditEditorModule::GetTabName()));
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				if (TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FPonyEditEditorModule::GetTabName())))
				{
					TSharedRef<SWidget> Content = Tab->GetContent();
					TSharedPtr<SPonyEditPanel> Panel = StaticCastSharedRef<SPonyEditPanel>(Content);
					Panel->SelectModule(4);
				}

				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
				{
					CapturePanelShot();
					FPlatformMisc::RequestExit(false);
					return false;
				}), 2.0f);

				return false;
			}), 3.0f);
		}));

	FAutoConsoleCommand GPonySelfTestScatterSceneCommand(
		TEXT("PonyEdit.SelfTestScatterScene"),
		TEXT("生成测试地表并撒一片，截关卡视口后退出。"),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
				UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
				if (!World || !Cube)
				{
					UE_LOG(LogPonyEdit, Warning, TEXT("[PonyEdit] ScatterScene: world or cube missing."));
					FPlatformMisc::RequestExit(false);
					return false;
				}

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AStaticMeshActor* Slab = World->SpawnActor<AStaticMeshActor>(
					AStaticMeshActor::StaticClass(), FVector(0.0, 0.0, -10.0), FRotator::ZeroRotator, SpawnParams);
				if (Slab)
				{
					Slab->GetStaticMeshComponent()->SetStaticMesh(Cube);
					Slab->SetActorScale3D(FVector(20.0, 20.0, 0.2));
					Slab->SetActorLabel(TEXT("M3_SceneSlab"));
				}

				GEditor->SelectNone(false, true);
				if (Slab)
				{
					GEditor->SelectActor(Slab, true, true);
				}

				const FString ScatterJson = TEXT(
					"{\"meshPath\":\"/Engine/BasicShapes/Cone.Cone\",\"count\":80,\"radius\":900,"
					"\"minScale\":0.7,\"maxScale\":1.5,\"maxSlopeDegrees\":30,\"clusterCount\":5,"
					"\"clusterRadius\":180,\"randomSeed\":20261023,\"dropToBase\":true}");
				const FString ScatterResult = UPonyEditScatterLibrary::ScatterOnSelection(ScatterJson);
				UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] ScatterScene: %s"), *ScatterResult);
				GEditor->SelectNone(false, true);

				if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
					ADirectionalLight::StaticClass(), FVector(0.0, 0.0, 1500.0), FRotator(-45.0, -30.0, 0.0), SpawnParams))
				{
					Sun->GetLightComponent()->SetIntensity(6.0f);
				}

				if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(
					ASkyLight::StaticClass(), FVector(0.0, 0.0, 500.0), FRotator::ZeroRotator, SpawnParams))
				{
					Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
					Sky->GetLightComponent()->SetIntensity(1.5f);
				}

				if (FViewport* Viewport = GEditor->GetActiveViewport())
				{
					if (FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetClient()))
					{
						ViewportClient->SetViewLocation(FVector(0.0, -2400.0, 1200.0));
						ViewportClient->SetViewRotation(FRotator(-18.0, 90.0, 0.0));
						ViewportClient->Invalidate();
					}
				}

				const FString ShotPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PonyEdit"), TEXT("scatter-scene.png"));
				IFileManager::Get().MakeDirectory(*FPaths::GetPath(ShotPath), true);
				FScreenshotRequest::RequestScreenshot(ShotPath, false, false, false);
				UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] ScatterScene shot requested: %s"), *ShotPath);

				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
				{
					FPlatformMisc::RequestExit(false);
					return false;
				}), 3.0f);

				return false;
			}), 2.0f);
		}));
}

void FPonyEditEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		GetTabName(),
		FOnSpawnTab::CreateRaw(this, &FPonyEditEditorModule::SpawnPanelTab))
		.SetDisplayName(LOCTEXT("TabTitle", "小马编辑"))
		.SetTooltipText(LOCTEXT("TabTooltip", "地编效率工具：导入、命名、材质、归底、种植、组合、地形、道路"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	MenusStartupHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPonyEditEditorModule::RegisterMenus));

	UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] Editor module started. Tab=%s, Engine=%d.%d"),
		*GetTabName().ToString(), ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);
}

void FPonyEditEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(MenusStartupHandle);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GetTabName());

	UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] Editor module shut down."));
}

void FPonyEditEditorModule::OpenPanel()
{
	UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] OpenPanel requested."));
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(GetTabName()));
}

TSharedRef<SDockTab> FPonyEditEditorModule::SpawnPanelTab(const FSpawnTabArgs& Args)
{
	UE_LOG(LogPonyEdit, Display, TEXT("[PonyEdit] Spawning panel tab."));

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SPonyEditPanel)
		];
}

void FPonyEditEditorModule::RegisterMenus()
{
	UToolMenus* Menus = UToolMenus::Get();
	if (!Menus)
	{
		return;
	}

	// 主菜单 Tools → 小马编辑（最稳的入口）
	if (UToolMenu* ToolsMenu = Menus->ExtendMenu("LevelEditor.MainMenu.Tools"))
	{
		FToolMenuSection& Section = ToolsMenu->FindOrAddSection("PonyEdit", LOCTEXT("PonyToolsSection", "小马编辑"));
		Section.AddMenuEntry(
			"PonyEdit.Open",
			LOCTEXT("OpenPonyEdit", "小马编辑"),
			LOCTEXT("OpenPonyEditTip", "打开小马编辑面板"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"),
			FUIAction(FExecuteAction::CreateRaw(this, &FPonyEditEditorModule::OpenPanel)));
	}

	// 工具栏按钮：PlayToolBar 是主目标，另一个是兜底，名字不存在时不会显示。
	const FName ToolbarMenus[] =
	{
		FName("LevelEditor.LevelEditorToolBar.PlayToolBar"),
		FName("LevelEditor.LevelEditorToolBar")
	};

	for (const FName& MenuName : ToolbarMenus)
	{
		if (UToolMenu* ToolbarMenu = Menus->ExtendMenu(MenuName))
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PonyEdit", LOCTEXT("PonyToolbarSection", "小马编辑"));
			Section.AddEntry(FToolMenuEntry::InitToolBarButton(
				MenuName == FName("LevelEditor.LevelEditorToolBar")
					? FName("PonyEdit.OpenButtonFallback")
					: FName("PonyEdit.OpenButton"),
				FUIAction(FExecuteAction::CreateRaw(this, &FPonyEditEditorModule::OpenPanel)),
				LOCTEXT("PonyButtonLabel", "小马"),
				LOCTEXT("PonyButtonTip", "打开小马编辑"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")));
		}
	}
}

#undef LOCTEXT_NAMESPACE
