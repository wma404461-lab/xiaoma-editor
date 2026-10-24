#include "PonyMeshOps.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Selection.h"
#include "ScopedTransaction.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
	void GatherSelectedActors(TArray<AActor*>& OutActors)
	{
		if (!GEditor)
		{
			return;
		}

		if (USelection* Selection = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator It(*Selection); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					OutActors.Add(Actor);
				}
			}
		}
	}

	void GatherSelectedStaticMeshes(TSet<UStaticMesh*>& OutMeshes)
	{
		TArray<AActor*> Actors;
		GatherSelectedActors(Actors);

		for (AActor* Actor : Actors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			TArray<UStaticMeshComponent*> Components;
			Actor->GetComponents<UStaticMeshComponent>(Components);
			for (UStaticMeshComponent* Component : Components)
			{
				if (Component && Component->GetStaticMesh())
				{
					OutMeshes.Add(Component->GetStaticMesh());
				}
			}
		}
	}
}

int32 FPonyMeshOps::SnapSelectedActorsToGround(FString& OutMessage)
{
	TArray<AActor*> Actors;
	GatherSelectedActors(Actors);

	if (Actors.Num() == 0)
	{
		OutMessage = TEXT("Select an actor, then click Align to Ground");
		return 0;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("PonyEdit", "SnapToGround", "Xiaoma Editor: Instance Align to Ground"));
	int32 MovedCount = 0;

	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		FBox WorldBounds(ForceInit);
		TArray<UStaticMeshComponent*> Components;
		Actor->GetComponents<UStaticMeshComponent>(Components);
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component && Component->IsRegistered())
			{
				WorldBounds += Component->Bounds.GetBox();
			}
		}

		if (!WorldBounds.IsValid)
		{
			continue;
		}

		const double DeltaZ = -WorldBounds.Min.Z;
		if (FMath::Abs(DeltaZ) < 0.01)
		{
			continue;
		}

		Actor->Modify();
		Actor->AddActorWorldOffset(FVector(0.0, 0.0, DeltaZ), false);
		Actor->MarkPackageDirty();
		++MovedCount;
	}

	OutMessage = FString::Printf(TEXT("Instances aligned to ground: %d / %d actors landed at Z=0"), MovedCount, Actors.Num());
	return MovedCount;
}

int32 FPonyMeshOps::PivotMeshesToBottom(FString& OutMessage)
{
	TSet<UStaticMesh*> Meshes;
	GatherSelectedStaticMeshes(Meshes);

	if (Meshes.Num() == 0)
	{
		OutMessage = TEXT("No StaticMesh selected on the chosen actor");
		return 0;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("PonyEdit", "PivotToBottom", "Xiaoma Editor: Pivot Align to Ground"));
	int32 ChangedCount = 0;
	int32 SkippedCount = 0;

	for (UStaticMesh* Mesh : Meshes)
	{
		if (!Mesh || !Mesh->IsMeshDescriptionValid(0))
		{
			++SkippedCount;
			continue;
		}

		Mesh->Modify();

		// calculate all valid ones first LOD of the merged bounding box.
		FBox Bounds(ForceInit);
		const int32 NumLODs = Mesh->GetNumLODs();
		for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
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

		if (!Bounds.IsValid)
		{
			++SkippedCount;
			continue;
		}

		const FVector Offset(
			-(Bounds.Min.X + Bounds.Max.X) * 0.5,
			-(Bounds.Min.Y + Bounds.Max.Y) * 0.5,
			-Bounds.Min.Z);

		if (Offset.IsNearlyZero())
		{
			++SkippedCount;
			continue;
		}

		for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
		{
			if (!Mesh->IsMeshDescriptionValid(LODIndex))
			{
				continue;
			}

			FMeshDescription* Description = Mesh->GetMeshDescription(LODIndex);
			if (!Description)
			{
				continue;
			}

			const auto Positions = Description->GetVertexPositions();
			for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
			{
				Positions.Set(VertexID, Positions.Get(VertexID) + FVector3f(Offset));
			}

			Mesh->CommitMeshDescription(LODIndex);
		}

		// simple collision follows translation; convex hull moves together, then physics rebuilt.
		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			for (FKBoxElem& Element : BodySetup->AggGeom.BoxElems)
			{
				Element.Center += Offset;
			}
			for (FKSphereElem& Element : BodySetup->AggGeom.SphereElems)
			{
				Element.Center += Offset;
			}
			for (FKSphylElem& Element : BodySetup->AggGeom.SphylElems)
			{
				Element.Center += Offset;
			}
			for (FKTaperedCapsuleElem& Element : BodySetup->AggGeom.TaperedCapsuleElems)
			{
				Element.Center += Offset;
			}
			for (FKConvexElem& Element : BodySetup->AggGeom.ConvexElems)
			{
				Element.SetTransform(FTransform(Offset) * Element.GetTransform());
			}

			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}

		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		++ChangedCount;

		// self-check: reading source mesh’s minZ, confirm vertices actually moved (render bounding box may have delayed refresh)).
		float SourceMinZ = TNumericLimits<float>::Max();
		if (const FMeshDescription* CheckDesc = Mesh->GetMeshDescription(0))
		{
			const auto Positions = CheckDesc->GetVertexPositions();
			for (const FVertexID VertexID : CheckDesc->Vertices().GetElementIDs())
			{
				SourceMinZ = FMath::Min(SourceMinZ, Positions.Get(VertexID).Z);
			}
		}

		UE_LOG(LogTemp, Display, TEXT("[PonyEdit] Pivot %s: offset=(%.1f, %.1f, %.1f), boundsBefore=%s, boundsAfter=%s, sourceMinZ=%.2f"),
			*Mesh->GetName(), Offset.X, Offset.Y, Offset.Z,
			*Bounds.ToString(), *Mesh->GetBoundingBox().ToString(), SourceMinZ);
	}

	OutMessage = FString::Printf(TEXT("Asset pivot aligned to ground: %d meshes processed, %d skipped (no available mesh description)"), ChangedCount, SkippedCount);
	return ChangedCount;
}
