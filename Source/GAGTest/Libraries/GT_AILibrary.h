#pragma once

#include "CoreMinimal.h"
#include "GAGTest/Utils/GT_Types.h"

#include "GT_AILibrary.generated.h"


UCLASS()
class UGT_AILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public: 

	static TArray<FVector> SmoothPath(AActor* WorldContextActor, FNavPathSharedPtr Path, FGT_SmoothPathConfig SmoothPathConfig = FGT_SmoothPathConfig());

	static TArray<FNavigationPortalEdge> GetPathPortalEdges(FNavPathSharedPtr Path);

	static bool ProjectPointToNavigation(AActor* WorldContextObject, const FVector& Point, FNavLocation& OutProjectedPoint, const FVector& Extent);

	static void DrawPath(AActor* WorldContextActor, TArray<FVector> Path, FLinearColor Color, float Duration, float OffsetZ);
};

