#include "VATModelAssetTypeActions.h"

#include "VATModel.h"
#include "VATModelEditorToolkit.h"

UClass* FVATModelAssetTypeActions::GetSupportedClass() const
{
	return UVATModel::StaticClass();
}

FText FVATModelAssetTypeActions::GetName() const
{
	return INVTEXT("VAT Model");
}

FColor FVATModelAssetTypeActions::GetTypeColor() const
{
	return FColor::Red;
}

uint32 FVATModelAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

void FVATModelAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	MakeShared<FVATModelEditorToolkit>()->InitEditor(InObjects);
}
