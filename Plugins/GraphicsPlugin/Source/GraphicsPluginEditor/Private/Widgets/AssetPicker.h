#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Components/ContentWidget.h"

#include "AssetPicker.generated.h"

class SObjectPropertyEntryBox;

UCLASS()
class UAssetPicker : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:

	// UVisual interface
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	// End of UVisual interface

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

private:
	bool GetResetVisibility(TSharedPtr<IPropertyHandle> PropertyHandle);
	void OnResetToBaseClicked(TSharedPtr<IPropertyHandle> PropertyHandle);
	FString GetCurrentAssetPath() const;
	void OnObjectChanged(const FAssetData& AssetData);

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetChanged, UObject*, Asset);
	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable, Category = "AssetPicker|Event")
	FOnAssetChanged OnAssetChanged;

	/** Called when the button is clicked */
	UPROPERTY(EditAnywhere, Category = "AssetPicker")
	UClass* AssetType = UObject::StaticClass();

protected:
	TSharedPtr<SObjectPropertyEntryBox> EntryBox;
	TWeakObjectPtr<UObject> SelectedAsset = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////