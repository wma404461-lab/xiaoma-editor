#include "PonyEditTestLibrary.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"

FBox UPonyEditTestLibrary::GetMeshSourceBounds(UStaticMesh* Mesh)
{
	FBox Bounds(ForceInit);
	if (!Mesh)
	{
		return Bounds;
	}

	for (int32 LODIndex = 0; LODIndex < Mesh->GetNumLODs(); ++LODIndex)
	{
		if (!Mesh->IsMeshDescriptionValid(LODIndex))
		{
			continue;
		}

		if (const FMeshDescription* Description = Mesh->GetMeshDescription(LODIndex))
		{
			const auto Positions = Description->GetVertexPositions();
			for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
			{
				Bounds += FVector(Positions.Get(VertexID));
			}
		}
	}

	return Bounds;
}

FString UPonyEditTestLibrary::GetTextureSettings(UTexture2D* Texture)
{
	if (!Texture)
	{
		return TEXT("null");
	}

	return FString::Printf(TEXT("srgb=%d;compression=%d;lodgroup=%d"),
		Texture->SRGB ? 1 : 0,
		static_cast<int32>(Texture->CompressionSettings),
		static_cast<int32>(Texture->LODGroup));
}

FString UPonyEditTestLibrary::GetMaterialCompileStatus(UMaterialInterface* Material)
{
	if (!Material)
	{
		return TEXT("exists=0");
	}

	const ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::SM5;
	UMaterial* BaseMaterial = Cast<UMaterial>(Material);
	if (!BaseMaterial)
	{
		BaseMaterial = Material->GetMaterial();
	}
	const bool bCompilingOrError = BaseMaterial
		? BaseMaterial->IsCompilingOrHadCompileError(FeatureLevel)
		: true;
	const FMaterialResource* Resource = Material->GetMaterialResource(FeatureLevel);
	return FString::Printf(TEXT("exists=1;compilingOrError=%d;hasResource=%d"),
		bCompilingOrError ? 1 : 0,
		Resource ? 1 : 0);
}
