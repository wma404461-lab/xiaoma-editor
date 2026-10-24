#pragma once

#include "CoreMinimal.h"

/**
 * Engine version check should be centralized here; do not scatter it in business code #if.
 * 5.6 Development baseline, 5.7 / 5.8 Only ensure buildable, panel accessible, and terrain chain runnable.
 */
#include "Runtime/Launch/Resources/Version.h"

#define PONYEDIT_ENGINE_AT_LEAST(Major, Minor) \
	(ENGINE_MAJOR_VERSION > (Major) || (ENGINE_MAJOR_VERSION == (Major) && ENGINE_MINOR_VERSION >= (Minor)))

// 5.8 Entering terrain editing layer is a mandatory path; 5.6 Still needs judgment「No layer / Has layer」.
#define PONYEDIT_LANDSCAPE_LAYERS_MANDATORY PONYEDIT_ENGINE_AT_LEAST(5, 8)
