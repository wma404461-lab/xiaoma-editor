#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PonyEditSettings.generated.h"

/**
 * 项目设置 → 插件 → 小马编辑。
 * 个人自用，配置跟着项目走，不做全局同步。
 */
UCLASS(config = PonyEdit, defaultconfig, meta = (DisplayName = "小马编辑"))
class PONYEDITRUNTIME_API UPonyEditSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPonyEditSettings();

	/** 批量、改名、改资产前自动存盘并保留回滚记录。 */
	UPROPERTY(EditAnywhere, config, Category = "安全", meta = (DisplayName = "安全模式"))
	bool bSafeMode = true;

	/** 资产导入的默认落点，相对 /Game。 */
	UPROPERTY(EditAnywhere, config, Category = "导入", meta = (DisplayName = "默认导入目录"))
	FString DefaultImportRoot = TEXT("/Game/Env");

	/** 监听落盘文件夹（M1 接入）。 */
	UPROPERTY(EditAnywhere, config, Category = "导入", meta = (DisplayName = "监听文件夹"))
	bool bWatchImportFolder = false;

	/** 资产级归底后，同步挪动关卡里已摆的实例，视觉不跳。 */
	UPROPERTY(EditAnywhere, config, Category = "整理", meta = (DisplayName = "归底时同步场景实例"))
	bool bFixupPlacedInstances = true;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
