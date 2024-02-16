#pragma once
#include "CoreMinimal.h"
#include "GAGTest/Utils/GT_Types.h"

#include "GT_QuerierActor.generated.h"

UCLASS()
class AGT_QuerierActor : public AActor
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditInstanceOnly, Category = "Path")
	AActor* TargetDestination = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Path")
	FGT_SmoothPathConfig SmoothPathConfig;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void FindPathToDestination();

};

