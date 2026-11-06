// AFireOnWord: the word-to-action mapping. Mirrors Assets/Fire/FireOnWord.cs in the Unity Fire sample.
// OnWord is where voice connects later: call it with each word the engine reports. It references no SDK code.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireOnWord.generated.h"

class AFireEffects;

UCLASS()
class FIREUNREAL_API AFireOnWord : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Fire")
	FString FireWord = TEXT("ファイア");

	UPROPERTY(EditAnywhere, Category = "Fire")
	FString StopWord = TEXT("ストップ");

	UPROPERTY(EditAnywhere, Category = "Fire")
	TObjectPtr<AFireEffects> Effects;

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void OnWord(FString Word);

	/** For the on-screen guide: the last word that arrived, and when (seconds of real time; negative: none yet). */
	const FString& GetLastWord() const { return LastWord; }
	double GetLastWordTime() const { return LastWordTime; }

private:
	FString LastWord;
	double LastWordTime = -1.0;
};
