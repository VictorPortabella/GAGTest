#pragma once

#include "CoreMinimal.h"

#include "GT_Types.generated.h"


USTRUCT(BlueprintType)
struct FGT_SmoothPathConfig
{
	GENERATED_BODY()

	//Strength with which we move away from the ledge. [0 - 1], when 0 is nothing and 1 is as much as possible
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float EdgeStrength = 0.25f;
	//Dist between samples to do the accuracy path. The smaller, the more samples, but the more it costs.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SampleDist = 200.f;
	//how many samples you take into account to make the smooth path.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int SensibilityNum = 5.f;
	//> 0.f, How much power you are giving the following samples to make the smooth path
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SensibilityStrength = 1.f;
	//Reduce the sensitivity strength if the path is not found. The smaller, the more accuracy it has, but the more it costs.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SensibilityAccuracy = 0.1f;
	//Draw Debug Lines.
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bDrawDebugLines = true;
};
