#include "PonyEditNamingLibrary.h"

#include "PonyNamingService.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString WriteJson(const TSharedRef<FJsonObject>& Root)
	{
		FString Output;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(Root, Writer);
		return Output;
	}

	TSharedPtr<FJsonObject> IssueToJson(const FPonyNamingIssue& Issue)
	{
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("current"), Issue.CurrentName);
		Object->SetStringField(TEXT("suggested"), Issue.SuggestedName);
		Object->SetStringField(TEXT("reason"), Issue.Reason);
		Object->SetNumberField(TEXT("severity"), Issue.Severity);
		Object->SetStringField(TEXT("path"), Issue.Asset.GetObjectPathString());
		return Object;
	}
}

FString UPonyEditNamingLibrary::ScanPath(const FString& ContentPath)
{
	TArray<FPonyNamingIssue> Issues;
	int32 Scanned = 0;
	FPonyNamingService::ScanPath(ContentPath, Issues, Scanned);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("scanned"), Scanned);
	Root->SetNumberField(TEXT("issueCount"), Issues.Num());

	TArray<TSharedPtr<FJsonValue>> IssueArray;
	for (const FPonyNamingIssue& Issue : Issues)
	{
		IssueArray.Add(MakeShared<FJsonValueObject>(IssueToJson(Issue)));
	}
	Root->SetArrayField(TEXT("issues"), IssueArray);

	return WriteJson(Root);
}

FString UPonyEditNamingLibrary::FixPath(const FString& ContentPath)
{
	TArray<FPonyNamingIssue> Issues;
	int32 Scanned = 0;
	FPonyNamingService::ScanPath(ContentPath, Issues, Scanned);

	TArray<FString> Renamed;
	TArray<FString> Failed;
	FPonyNamingService::ApplyFixes(Issues, Renamed, Failed);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("scanned"), Scanned);
	Root->SetNumberField(TEXT("renamedCount"), Renamed.Num());
	Root->SetNumberField(TEXT("failedCount"), Failed.Num());

	TArray<TSharedPtr<FJsonValue>> RenamedArray;
	for (const FString& Item : Renamed)
	{
		RenamedArray.Add(MakeShared<FJsonValueString>(Item));
	}
	Root->SetArrayField(TEXT("renamed"), RenamedArray);

	TArray<TSharedPtr<FJsonValue>> FailedArray;
	for (const FString& Item : Failed)
	{
		FailedArray.Add(MakeShared<FJsonValueString>(Item));
	}
	Root->SetArrayField(TEXT("failed"), FailedArray);

	return WriteJson(Root);
}

FString UPonyEditNamingLibrary::FixSelectedActorLabels()
{	TArray<FString> Renamed;
	FPonyNamingService::FixSelectedActorLabels(Renamed);
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("renamedCount"), Renamed.Num());

	TArray<TSharedPtr<FJsonValue>> RenamedArray;
	for (const FString& Item : Renamed)
	{
		RenamedArray.Add(MakeShared<FJsonValueString>(Item));
	}
	Root->SetArrayField(TEXT("renamed"), RenamedArray);

	return WriteJson(Root);
}

FString UPonyEditNamingLibrary::SuggestName(const FString& CurrentName, const FString& ClassName)
{
	FString Reason;
	const FString Suggested = FPonyNamingService::SuggestName(CurrentName, ClassName, Reason);
	return FString::Printf(TEXT("%s|%s"), *Suggested, *Reason);
}

