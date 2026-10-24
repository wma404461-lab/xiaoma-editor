#include "PonyEditScatterLibrary.h"

#include "CollisionQueryParams.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/DateTime.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	const TCHAR* GScatterTag = TEXT("PonyScatter");
	const TCHAR* GDefaultScatterMesh = TEXT("/Engine/BasicShapes/Cone.Cone");

	struct FPonyScatterParams
	{
		FString MeshPath = GDefaultScatterMesh;
		int32 Count = 40;
		float Radius = 500.f;
		float MinScale = 0.6f;
		float MaxScale = 1.4f;
		float MaxSlopeDegrees = 35.f;
		int32 ClusterCount = 4;
		float ClusterRadius = 120.f;
		float SurfaceOffset = 0.f;
		int32 RandomSeed = 0;
		FString SpawnMode = TEXT("actor");
		bool bAlignToNormal = true;
		bool bDropToBase = true;
		bool bHasCenter = false;
		FVector Center = FVector::ZeroVector;
		float TraceHeight = 1000.f;
	};

	struct FPonyScatterResult
	{
		bool bOk = false;
		FString Error;
		int32 Requested = 0;
		int32 Placed = 0;
		int32 ActorCount = 0;
		int32 SkippedNoHit = 0;
		int32 SkippedSlope = 0;
		int32 Seed = 0;
		FString SpawnMode = TEXT("actor");
		FVector Center = FVector::ZeroVector;
		float Radius = 0.f;
		int32 ClusterCount = 0;
		float ClusterRadius = 0.f;
		double ElapsedMs = 0.0;
		TArray<FVector> Positions;
		TArray<FVector2D> Clusters;
	};

	struct FPendingScatter
	{
		FVector Position = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		float Scale = 1.f;
		int32 Index = 0;
	};

	FString WriteJson(const TSharedRef<FJsonObject>& Root)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Root, Writer);
		return Output;
	}

	FString ErrorJson(const FString& Error)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), false);
		Root->SetStringField(TEXT("error"), Error);
		return WriteJson(Root);
	}

	FString GetStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, const FString& Default)
	{
		FString Value;
		return Object.IsValid() && Object->TryGetStringField(Field, Value) ? Value : Default;
	}

	double GetNumberField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, double Default)
	{
		double Value = 0.0;
		return Object.IsValid() && Object->TryGetNumberField(Field, Value) ? Value : Default;
	}

	bool GetBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, bool Default)
	{
		bool bValue = false;
		return Object.IsValid() && Object->TryGetBoolField(Field, bValue) ? bValue : Default;
	}

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	bool GetSelectedBounds(FBox& OutBounds)
	{
		OutBounds = FBox(ForceInit);
		if (!GEditor)
		{
			return false;
		}

		USelection* Selection = GEditor->GetSelectedActors();
		if (!Selection)
		{
			return false;
		}

		for (FSelectionIterator It(*Selection); It; ++It)
		{
			AActor* Actor = Cast<AActor>(*It);
			if (!IsValid(Actor))
			{
				continue;
			}

			FVector Origin = FVector::ZeroVector;
			FVector Extent = FVector::ZeroVector;
			Actor->GetActorBounds(false, Origin, Extent);
			if (Extent.IsNearlyZero())
			{
				continue;
			}

			OutBounds += FBox(Origin - Extent, Origin + Extent);
		}

		return OutBounds.IsValid != 0;
	}

	FVector2D RandomDiskPoint(FRandomStream& Random, float DiskRadius)
	{
		if (DiskRadius <= 0.f)
		{
			return FVector2D::ZeroVector;
		}

		const float Angle = Random.FRandRange(0.f, 2.f * PI);
		const float Distance = FMath::Sqrt(Random.FRand()) * DiskRadius;
		return FVector2D(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance);
	}

	void ParseParams(const TSharedPtr<FJsonObject>& Root, FPonyScatterParams& OutParams)
	{
		OutParams.MeshPath = GetStringField(Root, TEXT("meshPath"), GDefaultScatterMesh);
		OutParams.Count = FMath::Clamp(static_cast<int32>(GetNumberField(Root, TEXT("count"), 40.0)), 1, 500);
		OutParams.Radius = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("radius"), 500.0)), 10.f, 20000.f);
		OutParams.MinScale = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("minScale"), 0.6)), 0.01f, 100.f);
		OutParams.MaxScale = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("maxScale"), 1.4)), 0.01f, 100.f);
		if (OutParams.MaxScale < OutParams.MinScale)
		{
			Swap(OutParams.MinScale, OutParams.MaxScale);
		}
		OutParams.MaxSlopeDegrees = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("maxSlopeDegrees"), 35.0)), 0.f, 90.f);
		OutParams.ClusterCount = FMath::Clamp(static_cast<int32>(GetNumberField(Root, TEXT("clusterCount"), 4.0)), 0, OutParams.Count);
		OutParams.ClusterRadius = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("clusterRadius"), 120.0)), 0.f, OutParams.Radius);
		OutParams.SurfaceOffset = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("surfaceOffset"), 0.0)), -1000.f, 1000.f);
		OutParams.RandomSeed = static_cast<int32>(GetNumberField(Root, TEXT("randomSeed"), 0.0));
		OutParams.SpawnMode = GetStringField(Root, TEXT("spawnMode"), TEXT("actor")).ToLower();
		if (OutParams.SpawnMode != TEXT("hism"))
		{
			OutParams.SpawnMode = TEXT("actor");
		}
		OutParams.bAlignToNormal = GetBoolField(Root, TEXT("alignToNormal"), true);
		OutParams.bDropToBase = GetBoolField(Root, TEXT("dropToBase"), true);
		OutParams.TraceHeight = FMath::Clamp(static_cast<float>(GetNumberField(Root, TEXT("traceHeight"), 1000.0)), 10.f, 100000.f);

		const TArray<TSharedPtr<FJsonValue>>* CenterArray = nullptr;
		if (Root.IsValid() && Root->TryGetArrayField(TEXT("center"), CenterArray) && CenterArray && CenterArray->Num() >= 3)
		{
			OutParams.bHasCenter = true;
			OutParams.Center = FVector(
				(*CenterArray)[0]->AsNumber(),
				(*CenterArray)[1]->AsNumber(),
				(*CenterArray)[2]->AsNumber());
		}
	}

	TSharedRef<FJsonObject> SerializeResult(const FPonyScatterResult& Result)
	{
		const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), Result.bOk);
		if (!Result.bOk)
		{
			Root->SetStringField(TEXT("error"), Result.Error);
		}
		Root->SetNumberField(TEXT("requested"), Result.Requested);
		Root->SetNumberField(TEXT("placed"), Result.Placed);
		Root->SetNumberField(TEXT("actorCount"), Result.ActorCount);
		Root->SetStringField(TEXT("spawnMode"), Result.SpawnMode);
		Root->SetNumberField(TEXT("skippedNoHit"), Result.SkippedNoHit);
		Root->SetNumberField(TEXT("skippedSlope"), Result.SkippedSlope);
		Root->SetNumberField(TEXT("seed"), Result.Seed);
		Root->SetNumberField(TEXT("radius"), Result.Radius);
		Root->SetNumberField(TEXT("clusterCount"), Result.ClusterCount);
		Root->SetNumberField(TEXT("clusterRadius"), Result.ClusterRadius);
		Root->SetNumberField(TEXT("elapsedMs"), Result.ElapsedMs);

		TArray<TSharedPtr<FJsonValue>> CenterArray;
		CenterArray.Add(MakeShared<FJsonValueNumber>(Result.Center.X));
		CenterArray.Add(MakeShared<FJsonValueNumber>(Result.Center.Y));
		CenterArray.Add(MakeShared<FJsonValueNumber>(Result.Center.Z));
		Root->SetArrayField(TEXT("center"), CenterArray);

		TArray<TSharedPtr<FJsonValue>> ClusterArray;
		for (const FVector2D& Cluster : Result.Clusters)
		{
			TArray<TSharedPtr<FJsonValue>> Pair;
			Pair.Add(MakeShared<FJsonValueNumber>(Cluster.X));
			Pair.Add(MakeShared<FJsonValueNumber>(Cluster.Y));
			ClusterArray.Add(MakeShared<FJsonValueArray>(Pair));
		}
		Root->SetArrayField(TEXT("clusters"), ClusterArray);

		TArray<TSharedPtr<FJsonValue>> PositionArray;
		for (const FVector& Position : Result.Positions)
		{
			TArray<TSharedPtr<FJsonValue>> Point;
			Point.Add(MakeShared<FJsonValueNumber>(Position.X));
			Point.Add(MakeShared<FJsonValueNumber>(Position.Y));
			Point.Add(MakeShared<FJsonValueNumber>(Position.Z));
			PositionArray.Add(MakeShared<FJsonValueArray>(Point));
		}
		Root->SetArrayField(TEXT("positions"), PositionArray);
		return Root;
	}
}

FString UPonyEditScatterLibrary::ScatterOnSelection(const FString& ParamsJson)
{
	const double StartSeconds = FPlatformTime::Seconds();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ErrorJson(TEXT("Parameter is not valid JSON"));
	}

	FPonyScatterParams Params;
	ParseParams(Root, Params);

	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return ErrorJson(TEXT("No available editor world"));
	}

	UStaticMesh* Mesh = Cast<UStaticMesh>(LoadObject<UObject>(nullptr, *Params.MeshPath));
	if (!Mesh)
	{
		const FString ObjectPath = Params.MeshPath.Contains(TEXT("."))
			? Params.MeshPath
			: Params.MeshPath + TEXT(".") + FPackageName::GetShortName(Params.MeshPath);
		Mesh = Cast<UStaticMesh>(LoadObject<UObject>(nullptr, *ObjectPath));
	}
	if (!Mesh)
	{
		return ErrorJson(FString::Printf(TEXT("StaticMesh does not exist: %s"), *Params.MeshPath));
	}

	FBox Bounds(ForceInit);
	if (!GetSelectedBounds(Bounds))
	{
		if (!Params.bHasCenter)
		{
			return ErrorJson(TEXT("First select an actor that can be scattered, or set the center in the parameters"));
		}

		Bounds = FBox(Params.Center - FVector(0.0, 0.0, Params.TraceHeight), Params.Center + FVector(0.0, 0.0, Params.TraceHeight));
	}

	const FVector Center = Bounds.GetCenter();
	const float TraceHeight = FMath::Max(Params.TraceHeight, static_cast<float>(Bounds.GetExtent().Z) + 200.f);
	const double TopZ = Bounds.Max.Z + TraceHeight;
	const double BottomZ = Bounds.Min.Z - TraceHeight;

	FPonyScatterResult Result;
	Result.Requested = Params.Count;
	Result.Seed = Params.RandomSeed != 0 ? Params.RandomSeed : FMath::Rand();
	Result.Center = Center;
	Result.Radius = Params.Radius;
	Result.SpawnMode = Params.SpawnMode;
	Result.ClusterCount = Params.ClusterCount;
	Result.ClusterRadius = Params.ClusterRadius;

	FRandomStream Random(Result.Seed);
	for (int32 Index = 0; Index < Params.ClusterCount; ++Index)
	{
		const FVector2D Offset = RandomDiskPoint(Random, Params.Radius * 0.8f);
		Result.Clusters.Add(FVector2D(Center.X + Offset.X, Center.Y + Offset.Y));
	}

	const FBox MeshBounds = Mesh->GetBoundingBox();
	const FScopedTransaction Transaction(FText::FromString(TEXT("Xiaoma Edit: Surface Scatter")));

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(PonyScatterTrace), true);
	TArray<FPendingScatter> Pending;
	for (int32 Index = 0; Index < Params.Count; ++Index)
	{
		FVector2D Offset;
		if (Result.Clusters.Num() > 0)
		{
			const FVector2D& Cluster = Result.Clusters[Index % Result.Clusters.Num()];
			const FVector2D Local = RandomDiskPoint(Random, Params.ClusterRadius);
			Offset = FVector2D(Cluster.X + Local.X, Cluster.Y + Local.Y);
		}
		else
		{
			const FVector2D Local = RandomDiskPoint(Random, Params.Radius);
			Offset = FVector2D(Center.X + Local.X, Center.Y + Local.Y);
		}

		FHitResult Hit;
		const FVector TraceStart(Offset.X, Offset.Y, TopZ);
		const FVector TraceEnd(Offset.X, Offset.Y, BottomZ);
		if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
		{
			++Result.SkippedNoHit;
			continue;
		}

		const float NormalZ = FMath::Clamp(static_cast<float>(Hit.ImpactNormal.Z), -1.f, 1.f);
		const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(NormalZ));
		if (SlopeDegrees > Params.MaxSlopeDegrees)
		{
			++Result.SkippedSlope;
			continue;
		}

		const float Scale = Random.FRandRange(Params.MinScale, Params.MaxScale);
		const float BaseOffset = Params.bDropToBase && MeshBounds.IsValid
			? -MeshBounds.Min.Z * Scale
			: 0.f;
		const FVector Position = Hit.ImpactPoint + Hit.ImpactNormal * (Params.SurfaceOffset + BaseOffset);

		const float Yaw = Random.FRandRange(0.f, 360.f);
		FQuat FinalQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw));
		if (Params.bAlignToNormal)
		{
			FinalQuat = FQuat::FindBetweenNormals(FVector::UpVector, Hit.ImpactNormal) * FinalQuat;
		}
		const FRotator Rotation = FinalQuat.Rotator();

		FPendingScatter PendingSpawn;
		PendingSpawn.Position = Position;
		PendingSpawn.Rotation = Rotation;
		PendingSpawn.Scale = Scale;
		PendingSpawn.Index = Index;
		Pending.Add(PendingSpawn);
	}

	// sample first, then generate uniformly to avoid ray intersections with previously scattered instances.
	if (Params.SpawnMode == TEXT("hism") && Pending.Num() > 0)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Holder = World->SpawnActor<AActor>(
			AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (Holder)
		{
			UHierarchicalInstancedStaticMeshComponent* HISM =
				NewObject<UHierarchicalInstancedStaticMeshComponent>(Holder, TEXT("PonyScatterHISM"));
			Holder->SetRootComponent(HISM);
			HISM->SetMobility(EComponentMobility::Static);
			HISM->SetStaticMesh(Mesh);
			HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HISM->RegisterComponent();
			Holder->SetActorLabel(FString::Printf(TEXT("ENV_Scatter_HISM_%s"), *Mesh->GetName()));
			Holder->Tags.AddUnique(FName(GScatterTag));
			Holder->Modify();

			for (const FPendingScatter& PendingSpawn : Pending)
			{
				const FTransform InstanceTransform(
					PendingSpawn.Rotation, PendingSpawn.Position, FVector(PendingSpawn.Scale));
				if (HISM->AddInstance(InstanceTransform, true) != INDEX_NONE)
				{
					Result.Positions.Add(PendingSpawn.Position);
					++Result.Placed;
				}
				else
				{
					++Result.SkippedNoHit;
				}
			}

			Holder->MarkPackageDirty();
			Result.ActorCount = 1;
		}
		else
		{
			Result.SkippedNoHit += Pending.Num();
		}
	}
	else
	{
		for (const FPendingScatter& PendingSpawn : Pending)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* Spawned = World->SpawnActor<AStaticMeshActor>(
				AStaticMeshActor::StaticClass(), PendingSpawn.Position, PendingSpawn.Rotation, SpawnParams);
			if (!Spawned)
			{
				++Result.SkippedNoHit;
				continue;
			}

			Spawned->Modify();
			UStaticMeshComponent* Component = Spawned->GetStaticMeshComponent();
			Component->SetMobility(EComponentMobility::Static);
			Component->SetStaticMesh(Mesh);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Spawned->SetActorScale3D(FVector(PendingSpawn.Scale));
			Spawned->SetActorLabel(FString::Printf(TEXT("ENV_Scatter_%s_%02d"), *Mesh->GetName(), PendingSpawn.Index + 1));
			Spawned->Tags.AddUnique(FName(GScatterTag));
			Spawned->MarkPackageDirty();

			Result.Positions.Add(PendingSpawn.Position);
			++Result.Placed;
		}
		Result.ActorCount = Result.Placed;
	}

	World->MarkPackageDirty();
	Result.bOk = Result.Placed > 0;
	if (!Result.bOk)
	{
		Result.Error = TEXT("No instances placed (all missed or slope exceeded)");
	}
	Result.ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

	return WriteJson(SerializeResult(Result));
}

FString UPonyEditScatterLibrary::ClearScatter()
{
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return ErrorJson(TEXT("No available editor world"));
	}

	TArray<AActor*> ToDestroy;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->Tags.Contains(FName(GScatterTag)))
		{
			ToDestroy.Add(Actor);
		}
	}

	const FScopedTransaction Transaction(FText::FromString(TEXT("Xiaoma Edit: Clear Scatter")));
	int32 Removed = 0;
	for (AActor* Actor : ToDestroy)
	{
		if (World->DestroyActor(Actor))
		{
			++Removed;
		}
	}

	World->MarkPackageDirty();

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("ok"), true);
	Root->SetNumberField(TEXT("removed"), Removed);
	return WriteJson(Root);
}

FString UPonyEditScatterLibrary::GetScatterStats()
{
	UWorld* World = GetEditorWorld();
	if (!World)
	{
		return ErrorJson(TEXT("No available editor world"));
	}

	int32 ActorCount = 0;
	int32 InstanceCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || !Actor->Tags.Contains(FName(GScatterTag)))
		{
			continue;
		}

		++ActorCount;
		TArray<UHierarchicalInstancedStaticMeshComponent*> InstancedComponents;
		Actor->GetComponents<UHierarchicalInstancedStaticMeshComponent>(InstancedComponents);
		if (InstancedComponents.Num() > 0)
		{
			for (const UHierarchicalInstancedStaticMeshComponent* Component : InstancedComponents)
			{
				InstanceCount += Component ? Component->GetInstanceCount() : 0;
			}
		}
		else
		{
			++InstanceCount;
		}
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetBoolField(TEXT("ok"), true);
	Root->SetNumberField(TEXT("count"), InstanceCount);
	Root->SetNumberField(TEXT("actors"), ActorCount);
	return WriteJson(Root);
}
