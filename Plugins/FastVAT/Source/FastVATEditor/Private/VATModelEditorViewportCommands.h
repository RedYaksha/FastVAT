#pragma once

class FVATModelEditorViewportCommands : public TCommands<FVATModelEditorViewportCommands>
{
public:
	FVATModelEditorViewportCommands()
		: TCommands<FVATModelEditorViewportCommands>(
			TEXT("VATModelEditorViewport"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "VATModelEditorViewport", "VAT Model Editor Viewport"), // Localized context name for displaying
			NAME_None, // Parent
			FAppStyle::GetAppStyleSetName() // Icon Style Set
		)
	{}
	
	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> SingleStaticMesh;
	TSharedPtr<FUICommandInfo> NiagaraGrid;
	TSharedPtr<FUICommandInfo> ISMGrid;
	TSharedPtr<FUICommandInfo> HISMGrid;
};
