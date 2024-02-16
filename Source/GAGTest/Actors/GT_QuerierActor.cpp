#include "GT_QuerierActor.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "GAGTest/Libraries/GT_AILibrary.h"
#include "Runtime/Core/Public/Stats/Stats2.h"


void AGT_QuerierActor::BeginPlay()
{
	Super::BeginPlay();

}

void AGT_QuerierActor::FindPathToDestination()
{
	if(!TargetDestination)
	{
		UE_LOG(LogTemp, Warning, TEXT("AGT_QuerierActor::FindPathToDestination TargetDestination is nullptr"));
		return;
	}

	const FVector PathStart = GetActorLocation();
	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, PathStart, TargetDestination);
	if(!NavPath)
		return;


	TArray<FVector> SmoothPath = UGT_AILibrary::SmoothPath(this, NavPath->GetPath(), SmoothPathConfig);
}
