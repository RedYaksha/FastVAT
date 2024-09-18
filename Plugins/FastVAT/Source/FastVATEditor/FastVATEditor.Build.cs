using UnrealBuildTool;

public class FastVATEditor : ModuleRules
{
    public FastVATEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", 
                "AssetTools", 
                "FastVAT", 
                "EditorScriptingUtilities", 
                "MaterialEditor", 
                "StaticMeshDescription", 
                "MeshDescription",
                "AdvancedPreviewScene",
                "ToolMenus",
                "Niagara",
                "InputCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "UnrealEd", 
            }
        );
    }
}