#include "GT_AILibrary.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavMesh/NavMeshPath.h"
#include "Kismet/KismetMathLibrary.h"
#include "Runtime/Core/Public/Stats/Stats2.h"

DECLARE_STATS_GROUP(TEXT("StatGroup_AI"), StatGroup_AI, StatGroup_AI)

TArray<FVector> UGT_AILibrary::SmoothPath(AActor* WorldContextActor, FNavPathSharedPtr Path, FGT_SmoothPathConfig SmoothPathConfig)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("STAT_SmoothPath"), STAT_SmoothPath, StatGroup_AI);

	TArray<FVector> OutSmoothPath;

	if (!ensure(Path != nullptr && Path->IsValid() && WorldContextActor))
	{
		return OutSmoothPath;
	}

	if (!ensureMsgf((SmoothPathConfig.EdgeStrength >= 0.f && SmoothPathConfig.EdgeStrength <= 1.f), TEXT("SmoothPathConfig.EdgeStrength must be [0.f, 1.f]")) ||
		!ensureMsgf((SmoothPathConfig.SensibilityStrength > 0.f), TEXT("SmoothPathConfig.SensibilityStrength must be > 0.f")))
	{
		return OutSmoothPath;
	}

	const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
	const TArray<FNavigationPortalEdge>& PathPortalEdges = GetPathPortalEdges(Path);

	//Get desired edges points in Path Portal Edges. We consider EdgeStrength to get away from corners.
	TArray<FVector> EdgesPath;
	EdgesPath.Add(PathPoints[0]);
	int CurrentEdge = 0;
	for (int i = 1; i < PathPoints.Num() - 1; ++i)
	{
		const FNavPathPoint& CurrentPathPoint = PathPoints[i];
		for (CurrentEdge; CurrentEdge < PathPortalEdges.Num(); CurrentEdge++)
		{
			//Firstly we look if the Current Edge is associated with an original path point
			const FNavigationPortalEdge& CurrentPathEdge = PathPortalEdges[CurrentEdge];
			if (CurrentPathPoint.NodeRef == CurrentPathEdge.ToRef)
			{
				const FVector StartPoint = CurrentPathPoint;
				const FVector EdgeDir = UKismetMathLibrary::GetDirectionUnitVector(StartPoint, CurrentPathEdge.GetMiddlePoint());
				const FVector EdgeFinalPoint = StartPoint + EdgeDir * CurrentPathEdge.GetLength();

				const FVector DesiredEdgePoint = UKismetMathLibrary::VLerp(StartPoint, EdgeFinalPoint, SmoothPathConfig.EdgeStrength);
				EdgesPath.Add(DesiredEdgePoint);
				CurrentEdge++;

				break;
			}

			//If it is not associated, we look for the intersection point with the original path.
			const FVector LastPathPointLocation = PathPoints[i - 1];
			const FVector NextPathPointLocation = CurrentPathPoint;
			const FVector EdgeStart = CurrentPathEdge.GetPoint(0);
			const FVector EdgeEnd = CurrentPathEdge.GetPoint(1);
			FVector OutIntersection;
			FMath::SegmentIntersection2D(LastPathPointLocation, NextPathPointLocation, EdgeStart, EdgeEnd, OutIntersection);

			const FVector EdgeDir = UKismetMathLibrary::GetDirectionUnitVector(OutIntersection, CurrentPathEdge.GetMiddlePoint());

			//Adjusted Edge from the IntersectionPoint to the Edge End
			const float AdjustedEdgeLenght = CurrentPathEdge.GetLength() - FMath::Min(FVector::Dist(OutIntersection, CurrentPathEdge.GetPoint(0)), FVector::Dist(OutIntersection, CurrentPathEdge.GetPoint(1)));
			const FVector AdjustedEdgeEnd = OutIntersection + EdgeDir * AdjustedEdgeLenght;
			const FVector DesiredEdgePoint = UKismetMathLibrary::VLerp(OutIntersection, AdjustedEdgeEnd, SmoothPathConfig.EdgeStrength);

			EdgesPath.Add(DesiredEdgePoint);
		}
	}
	EdgesPath.Add(PathPoints[PathPoints.Num() - 1]);

	// Once we have our path far from the corners, we sample it in order to have more accuracy
	TArray<FVector> SampledPath;
	for (int i = 0; i < EdgesPath.Num() - 1; i++)
	{
		const FVector& CurrentPoint = EdgesPath[i];
		const FVector& NextPoint = EdgesPath[i + 1];

		const FVector DirToNextPoint = UKismetMathLibrary::GetDirectionUnitVector(CurrentPoint, NextPoint);

		SampledPath.Add(CurrentPoint);

		float DistToNextPoint = FVector::Dist(CurrentPoint, NextPoint);
		int Count = 1;
		while (DistToNextPoint > SmoothPathConfig.SampleDist)
		{
			SampledPath.Add(CurrentPoint + DirToNextPoint * SmoothPathConfig.SampleDist * Count);

			DistToNextPoint -= SmoothPathConfig.SampleDist;
			Count++;
		}
	}
	SampledPath.Add(EdgesPath[EdgesPath.Num() - 1]);

	//Now, we want to get our Smooth Path, lerping our point position with the next ones.
	//We check that we are in navmesh the whole path
	OutSmoothPath.Add(SampledPath[0]);
	for (int i = 1; i < SampledPath.Num(); i++)
	{
		float CurrentSensibilityStrength = SmoothPathConfig.SensibilityStrength;
		bool bSuccess = false;

		while (!bSuccess)
		{
			FVector SmoothPosition = SampledPath[i];
			for (int j = 1; j <= SmoothPathConfig.SensibilityNum; j++)
			{
				if (i + j < SampledPath.Num())
				{
					SmoothPosition = UKismetMathLibrary::VLerp(SmoothPosition, SampledPath[i + j], 1.f / FMath::Pow(2.f, 1.f / CurrentSensibilityStrength * j));
				}
			}

			FNavLocation ProjectedSmoothPosition;
			const FVector Extent = FVector(100.f);
			ProjectPointToNavigation(WorldContextActor, SmoothPosition, ProjectedSmoothPosition, Extent);
			
			FVector OutHitLocation;
			bSuccess = !UNavigationSystemV1::NavigationRaycast(WorldContextActor, OutSmoothPath[i - 1], ProjectedSmoothPosition.Location, OutHitLocation);
			
			if (bSuccess)
			{
				OutSmoothPath.Add(ProjectedSmoothPosition.Location);
			}
			else
			{
				if (SmoothPathConfig.bDrawDebugLines)
				{
					DrawDebugLine(WorldContextActor->GetWorld(), OutSmoothPath[i - 1] + FVector::UpVector * 15, OutHitLocation + FVector::UpVector * 15, FLinearColor::Black.ToFColor(true), false, 100.f, 0, 5.f);
				}

				CurrentSensibilityStrength -= SmoothPathConfig.SensibilityAccuracy;
				if (CurrentSensibilityStrength <= 0)
				{
					break;
				}
			}
		}

		if (!bSuccess)
		{
			break;
		}
	}

	if (SmoothPathConfig.bDrawDebugLines)
	{
		//Draw OrigPathPoints in Blue
		TArray<FVector> OrigPathPointsLocation;
		for (FNavPathPoint PathPoint : PathPoints)
		{
			OrigPathPointsLocation.Add(PathPoint.Location);
		}
		UGT_AILibrary::DrawPath(WorldContextActor, OrigPathPointsLocation, FLinearColor::Blue, 0.1f, 2.f);

		//Draw Sampled Path Points in Red
		//UGT_AILibrary::DrawPath(WorldContextActor, SampledPath, FLinearColor::Red, 0.1f, 5.f);

		//Draw Smooth Path Points in Green
		UGT_AILibrary::DrawPath(WorldContextActor, OutSmoothPath, FLinearColor::Green, 0.1f, 10.f);
	}

	return OutSmoothPath;
}

TArray<FNavigationPortalEdge> UGT_AILibrary::GetPathPortalEdges(FNavPathSharedPtr Path)
{
	TArray<FNavigationPortalEdge> PortalEdges;
	if (!ensure(Path != nullptr) && Path->IsValid())
	{
		return PortalEdges;
	}

	FNavMeshPath* NavMeshPath = static_cast<FNavMeshPath*>(Path.Get());
	if (!ensure(NavMeshPath))
	{
		return PortalEdges;
	}

	return NavMeshPath->GetPathCorridorEdges();
}

bool UGT_AILibrary::ProjectPointToNavigation(AActor* WorldContextObject, const FVector& Point, FNavLocation& OutProjectedPoint, const FVector& Extent)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(WorldContextObject->GetWorld());
	if (!NavSystem)
	{
		return false;
	}

	return NavSystem->ProjectPointToNavigation(Point, OutProjectedPoint, Extent);
}

void UGT_AILibrary::DrawPath(AActor* WorldContextActor, TArray<FVector> Path, FLinearColor Color, float Duration, float OffsetZ)
{
#if WITH_EDITOR
	for (int i = 0; i + 1 < Path.Num(); ++i)
	{
		DrawDebugPoint(WorldContextActor->GetWorld(), Path[i] + FVector::UpVector * OffsetZ + 1.f, 10.f, Color.ToFColor(true), false, Duration, 0);
		DrawDebugLine(WorldContextActor->GetWorld(), Path[i] + FVector::UpVector * OffsetZ, Path[i + 1] + FVector::UpVector * OffsetZ, Color.ToFColor(true), false, Duration, 0, 5.f);
	}
#endif
}


