#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Delegates/Delegate.h"

class SDockTab;
class FSpawnTabArgs;

class FPonyEditEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FName GetTabName() { return TEXT("PonyEditPanel"); }

	void OpenPanel();

private:
	TSharedRef<SDockTab> SpawnPanelTab(const FSpawnTabArgs& Args);
	void RegisterMenus();
	void RegisterConsoleCommands();

	FDelegateHandle MenusStartupHandle;
};
