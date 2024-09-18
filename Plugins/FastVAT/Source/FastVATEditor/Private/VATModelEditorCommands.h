#pragma once
#include "Framework/Commands/Commands.h"

class FVATModelEditorCommands : public TCommands<FVATModelEditorCommands>
{
public:
	FVATModelEditorCommands()
		: TCommands<FVATModelEditorCommands>(
			TEXT("VATModelEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "VATModelEditor", "VAT Model Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FAppStyle::GetAppStyleSetName() // Icon Style Set
		)
	{}
	
	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> GenerateVertexAnimationTextures;
};
