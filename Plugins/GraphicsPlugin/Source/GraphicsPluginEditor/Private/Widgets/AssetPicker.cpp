#include "AssetPicker.h"
#include "LevelEditor.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UAssetPicker

UAssetPicker::UAssetPicker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = true;
	Visibility = ESlateVisibility::SelfHitTestInvisible;
}

void UAssetPicker::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	EntryBox.Reset();
}

TSharedRef<SWidget> UAssetPicker::RebuildWidget()
{
	FResetToDefaultOverride ResetToDefaultOverride = FResetToDefaultOverride::Create(
		FIsResetToDefaultVisible::CreateUObject(this, &UAssetPicker::GetResetVisibility),
		FResetToDefaultHandler::CreateUObject(this, &UAssetPicker::OnResetToBaseClicked)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool = LevelEditorModule.GetFirstLevelEditor()->GetThumbnailPool();

	EntryBox = SNew(SObjectPropertyEntryBox)
		.AllowedClass(AssetType)
		.AllowClear(true)
		.CustomResetToDefault(ResetToDefaultOverride)
		.ThumbnailPool(ThumbnailPool)
		.ObjectPath_UObject(this, &UAssetPicker::GetCurrentAssetPath)
		.OnObjectChanged_UObject(this, &UAssetPicker::OnObjectChanged);

	return EntryBox.ToSharedRef();
}

bool UAssetPicker::GetResetVisibility(TSharedPtr<IPropertyHandle> PropertyHandle)
{
	return SelectedAsset.IsValid();
}

void UAssetPicker::OnResetToBaseClicked(TSharedPtr<IPropertyHandle> PropertyHandle)
{
	SelectedAsset = nullptr;
	OnAssetChanged.Broadcast(SelectedAsset.Get());
}

FString UAssetPicker::GetCurrentAssetPath() const
{
	return SelectedAsset.IsValid() ? SelectedAsset->GetPathName() : FString("");
}

void UAssetPicker::OnObjectChanged(const FAssetData& AssetData)
{
	SelectedAsset = AssetData.GetAsset();
	OnAssetChanged.Broadcast(SelectedAsset.Get());
}

#if WITH_EDITOR

const FText UAssetPicker::GetPaletteCategory()
{
	return LOCTEXT("Editor", "Editor");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
