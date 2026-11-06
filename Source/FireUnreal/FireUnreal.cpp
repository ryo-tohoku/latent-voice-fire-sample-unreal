#include "FireUnreal.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR && !WITH_LATENT_VOICE
#include "DesktopPlatformModule.h"
#include "Misc/CoreDelegates.h"
#include "Misc/MessageDialog.h"
#include "Misc/FeedbackContext.h"
#include "Misc/Paths.h"
#include "UnrealEdMisc.h"
#endif

DEFINE_LOG_CATEGORY(LogFire);

// The sample opens without the Latent Voice plugin (FireUnreal.Build.cs, the .uproject lists it as optional). Unreal does not offer to build an
// optional plugin that appears later, so when the developer has put it into Plugins/ and this build is still the one without it, the Editor asks
// here, builds the project as its own "rebuild the missing modules" does, and restarts.
class FFireUnrealModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR && !WITH_LATENT_VOICE
		if (GIsEditor && !IsRunningCommandlet())
		{
			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FFireUnrealModule::OfferToBuildThePlugin);
		}
#endif
	}

#if WITH_EDITOR && !WITH_LATENT_VOICE
	void OfferToBuildThePlugin()
	{
		if (!FPaths::FileExists(FPaths::ProjectPluginsDir() / TEXT("LatentVoice/LatentVoice.uplugin"))) return;
		const FText Ask = FText::FromString(TEXT("The Latent Voice plugin is in Plugins/LatentVoice, but this project was built without it.\n\nBuild the project with the plugin now? The Editor restarts when it is done (about a minute)."));
		if (FMessageDialog::Open(EAppMsgType::YesNo, Ask, FText::FromString(TEXT("Latent Voice"))) != EAppReturnType::Yes) return;
		IDesktopPlatform* Desktop = FDesktopPlatformModule::Get();
		if (!Desktop || !Desktop->CompileGameProject(FPaths::RootDir(), FPaths::GetProjectFilePath(), GWarn))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("The build did not succeed. The Output Log has the details. Close the Editor and open FireUnreal.uproject again to retry.")));
			return;
		}
		FUnrealEdMisc::Get().RestartEditor(false);
	}
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFireUnrealModule, FireUnreal, "FireUnreal");
