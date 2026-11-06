using UnrealBuildTool;

public class FireUnreal : ModuleRules
{
	public FireUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Voice: the Latent Voice plugin, from the developer portal, in Plugins/LatentVoice. Optional: without it the project still opens and plays,
		// and the guide on the screen says how to add it (the .uproject lists it as optional).
		bool bVoice = System.IO.File.Exists(System.IO.Path.Combine(ModuleDirectory, "..", "..", "Plugins", "LatentVoice", "LatentVoice.uplugin"));
		if (bVoice) PrivateDependencyModuleNames.Add("LatentVoice");
		PublicDefinitions.Add("WITH_LATENT_VOICE=" + (bVoice ? "1" : "0"));
		// The Editor's offer to build the plugin once it is in Plugins/ (FireUnreal.cpp)
		if (Target.bBuildEditor && !bVoice) PrivateDependencyModuleNames.AddRange(new string[] { "DesktopPlatform", "UnrealEd" });
	}
}
