#include "FireOnWord.h"

#include "Engine/World.h"
#include "FireEffects.h"
#include "FireUnreal.h"

void AFireOnWord::OnWord(FString Word)
{
	LastWord = Word;
	LastWordTime = GetWorld()->GetRealTimeSeconds();
	UE_LOG(LogFire, Log, TEXT("[Fire sample] heard 「%s」"), *Word);

	if (Word == FireWord) Effects->Light();
	else if (Word == StopWord) Effects->PutOut();
}
