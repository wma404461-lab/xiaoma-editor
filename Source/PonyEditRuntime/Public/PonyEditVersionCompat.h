#pragma once

#include "CoreMinimal.h"

/**
 * 引擎版本判断集中在这里，业务代码里不要再散落 #if。
 * 5.6 是开发基线，5.7 / 5.8 只保证能编、能开面板、地形链路能跑。
 */
#include "Runtime/Launch/Resources/Version.h"

#define PONYEDIT_ENGINE_AT_LEAST(Major, Minor) \
	(ENGINE_MAJOR_VERSION > (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION >= (Minor)))

// 5.8 起地形编辑层是必需路径；5.6 仍需判断「无层 / 有层」。
#define PONYEDIT_LANDSCAPE_LAYERS_MANDATORY PONYEDIT_ENGINE_AT_LEAST(5, 8)
