#include "FastVATEditorModule.h"

#include "VATModelEditorCommands.h"
#include "VATModelEditorViewportCommands.h"

#define LOCTEXT_NAMESPACE "FFastVATEditorModule"

void FFastVATEditorModule::StartupModule()
{
	VATModelAssetTypeActions = MakeShared<FVATModelAssetTypeActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(VATModelAssetTypeActions.ToSharedRef());
	
	// Register commands
	FVATModelEditorCommands::Register();
	FVATModelEditorViewportCommands::Register();
}

void FFastVATEditorModule::ShutdownModule()
{
	// Unregister commands
	FVATModelEditorCommands::Unregister();
	FVATModelEditorViewportCommands::Unregister();
	
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		return;
	}
	
	FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(VATModelAssetTypeActions.ToSharedRef()); 
	
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FFastVATEditorModule, FastVATEditor)