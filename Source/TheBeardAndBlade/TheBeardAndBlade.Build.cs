using UnrealBuildTool;
// Canvas room scrolling is compiled with the gameplay module; no extra plugins.
public class TheBeardAndBlade : ModuleRules
{
    public TheBeardAndBlade(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // File-local rendering helpers must remain separate translation units,
        // regardless of whether Git considers the source modified or committed.
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RenderCore", "RHI", "Niagara" });
        if (Target.bBuildEditor) {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MaterialEditor" });
            PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory,"Plugins/FX/Niagara/Source/Niagara/Internal"));
            PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory,"Plugins/FX/Niagara/Source/NiagaraShader/Internal"));
            PrivateDependencyModuleNames.Add("NiagaraShader");
        }
    }
}
