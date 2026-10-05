using UnrealBuildTool;
// Canvas room scrolling is compiled with the gameplay module; no extra plugins.
public class TheBeardAndBlade : ModuleRules { public TheBeardAndBlade(ReadOnlyTargetRules Target) : base(Target) { PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs; PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" }); PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RenderCore", "RHI" }); } }
