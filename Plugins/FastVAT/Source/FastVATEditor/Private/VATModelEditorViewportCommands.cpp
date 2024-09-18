#include "VATModelEditorViewportCommands.h"

#define LOCTEXT_NAMESPACE "VATModelEditorViewportCommands"

void FVATModelEditorViewportCommands::RegisterCommands()
{
	UI_COMMAND( SingleStaticMesh, "Single Mesh", "Renders the VAT static mesh as a UStaticMeshComponent", EUserInterfaceActionType::RadioButton, FInputChord() );
	UI_COMMAND( NiagaraGrid, "Niagara Grid", "Renders the VAT static mesh with a Niagara GPU system", EUserInterfaceActionType::RadioButton, FInputChord() );
}

#undef LOCTEXT_NAMESPACE