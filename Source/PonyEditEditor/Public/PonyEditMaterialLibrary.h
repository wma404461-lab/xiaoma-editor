#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PonyEditMaterialLibrary.generated.h"

/**
 * M2 Material rack: 8 A set of standard master material parameter contracts, MI Generate, contract validation, and material hierarchy query.
 *
 * Return to all entry points JSON, for convenient panel display and Python automated assertions.
 * Master material path fixed /Game/PonyEdit/Materials/Masters, MI Default placement /Game/PonyEdit/Materials/Instances.
 */
UCLASS()
class PONYEDITEDITOR_API UPonyEditMaterialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Material rack: 8 Check if master materials exist, whether contracts are complete, and how many each has MI. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString GetRack();

	/** Check individual master material parameter contracts and return missing ones/Extra parameters. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString ValidateMaster(const FString& MasterPath);

	/**
	 * Create (or reuse) a set based on master material contracts)MI, Apply contract default parameters and save.
	 * Name does not start with MI_ Automatically prepend prefix when name doesn’t start with.
	 */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString CreateInstance(const FString& MasterPath, const FString& PackagePath, const FString& AssetName);

	/** Using selected Actor mesh name to create MI (MI_<Mesh>_<Type>), And directly apply to selected Actor. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString CreateInstanceForSelected(const FString& MasterPath, const FString& PackagePath);

	/** Apply material to selected Actor; SlotIndex < 0 Represents all slots. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString ApplyToSelected(const FString& MaterialPath, int32 SlotIndex);

	/** Read MI Parent and current contract parameter values, for panel/Automated validation. */
	UFUNCTION(BlueprintCallable, Category = "PonyEdit|Material")
	static FString GetInstanceInfo(const FString& MaterialPath);
};
