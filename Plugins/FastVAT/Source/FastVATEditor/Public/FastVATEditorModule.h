#pragma once

#include "CoreMinimal.h"
#include "VATModelAssetTypeActions.h"
#include "Modules/ModuleManager.h"

class FFastVATEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
	TSharedPtr<FVATModelAssetTypeActions> VATModelAssetTypeActions;
	
};
