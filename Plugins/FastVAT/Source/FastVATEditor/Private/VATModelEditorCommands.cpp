#include "VATModelEditorCommands.h"

#define LOCTEXT_NAMESPACE "VATModelEditorCommands"

void FVATModelEditorCommands::RegisterCommands()
{
	UI_COMMAND(GenerateVertexAnimationTextures, "Generate VAT", "Execute the vertex animation texture automation process", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
