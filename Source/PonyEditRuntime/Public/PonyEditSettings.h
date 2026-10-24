#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PonyEditSettings.generated.h"

/**
 * project settings → plugins → Xiaoma Editor.
 * personal use, configuration follows project, no global sync.
 */
UCLASS(config = PonyEdit, defaultconfig, meta = (DisplayName = "Xiaoma Editor"))
class PONYEDITRUNTIME_API UPonyEditSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPonyEditSettings();

	/** auto-save before batch operations, renaming, or asset changes, and retain rollback records. */
	UPROPERTY(EditAnywhere, config, Category = "safe", meta = (DisplayName = "safe mode"))
	bool bSafeMode = true;

	/** default asset drop location, relative to /Game. */
	UPROPERTY(EditAnywhere, config, Category = "Import", meta = (DisplayName = "default import directory"))
	FString DefaultImportRoot = TEXT("/Game/Env");

	/** watch save folder (M1 integrate). */
	UPROPERTY(EditAnywhere, config, Category = "Import", meta = (DisplayName = "Monitor folder"))
	bool bWatchImportFolder = false;

	/** After asset consolidation, sync instances in levels already placed there; visual continuity without jumps. */
	UPROPERTY(EditAnywhere, config, Category = "Organize", meta = (DisplayName = "Sync scene instances during consolidation"))
	bool bFixupPlacedInstances = true;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
