// Fill out your copyright notice in the Description page of Project Settings.


#include "ETW_MassMoveToCursor.h"
#include "MassEntityTemplateRegistry.h"
#include "MassMovementFragments.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationFragments.h"
#include "MassObserverRegistry.h"
#include "MassSimulationLOD.h"
#include "../Common/Fragments/ETW_MassFragments.h"
#include "MassCommanderComponent.h"
#include "Kismet/GameplayStatics.h"
#include <Translators/MassCharacterMovementTranslators.h>
#include "GameFramework/CharacterMovementComponent.h"
#include "MassCommands.h"

void UETW_MassMoveToCursorTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
	const UWorld& World) const
{
	BuildContext.RequireFragment<FMassVelocityFragment>();
	//BuildContext.RequireFragment<FMassForceFragment>();
	BuildContext.RequireFragment<FTransformFragment>();

	//BuildContext.RequireFragment<FMassMovementParameters>();
	
	BuildContext.AddFragment<FMassMoveToCursorFragment>();

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	const FConstSharedStruct ParamsFragment = EntityManager.GetOrCreateConstSharedFragment(Params);
	BuildContext.AddConstSharedFragment(ParamsFragment);
}

UETW_MassMoveToCursorProcessor::UETW_MassMoveToCursorProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Avoidance);
}

void UETW_MassMoveToCursorProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	//EntityQuery.AddRequirement<FMassForceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveToCursorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveToCursorCommanderFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FCharacterMovementComponentWrapperFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);

	EntityQuery.AddConstSharedRequirement<FMassMoveToCursorParams>(EMassFragmentPresence::All);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>(EMassFragmentPresence::Optional);
}

void UETW_MassMoveToCursorProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, ([](FMassExecutionContext& Context)
		{
			const FMassMoveToCursorParams MoveToCursorParams = Context.GetConstSharedFragment<FMassMoveToCursorParams>();
			const float ArriveSlackRadius = MoveToCursorParams.TargetReachThreshold;
			const FMassMovementParameters* MovementParams = Context.GetConstSharedFragmentPtr<FMassMovementParameters>();
			
			const TConstArrayView<FCharacterMovementComponentWrapperFragment> CharacterMovementList = Context.GetFragmentView<FCharacterMovementComponentWrapperFragment>();
			bool bHasMovementComponent = CharacterMovementList.Num() > 0;

			const float Speed = MovementParams ? MovementParams->MaxSpeed : MoveToCursorParams.SpeedFallbackValue;
		
			const TArrayView<FMassVelocityFragment> VelocityList = Context.GetMutableFragmentView<FMassVelocityFragment>();
			//const TArrayView<FMassForceFragment> ForceList = Context.GetMutableFragmentView<FMassForceFragment>();
			const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();
			const TArrayView<FMassMoveToCursorFragment> MoveToCursorList = Context.GetMutableFragmentView<FMassMoveToCursorFragment>();
			const TArrayView<FMassMoveToCursorCommanderFragment> MoveToCursorCommanderList = Context.GetMutableFragmentView<FMassMoveToCursorCommanderFragment>();

			for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
			{
				UMassCommanderComponent* CommanderComp = MoveToCursorCommanderList[EntityIndex].CommanderComp;
				if (!CommanderComp)
				{
					continue;
				}

				float EntitySpeed = Speed;
				if (bHasMovementComponent && CharacterMovementList[EntityIndex].Component.IsValid())
				{
					EntitySpeed = CharacterMovementList[EntityIndex].Component->GetMaxSpeed();
				}
				
				FMassMoveToCursorFragment& MoveToCursor = MoveToCursorList[EntityIndex];

				//FVector& Force = ForceList[EntityIndex].Value;
				FVector& Velocity = VelocityList[EntityIndex].Value;
				FTransform& Transform = TransformList[EntityIndex].GetMutableTransform();
				const FVector& CurrentLocation = Transform.GetLocation();
				FVector& TargetLocation = MoveToCursor.Target;

				MoveToCursor.Timer += Context.GetDeltaTimeSeconds();
				
				if (MoveToCursor.Timer >= 1.f || FVector::Dist(TargetLocation, CurrentLocation) <= ArriveSlackRadius || TargetLocation.IsZero())
				{
					Velocity = FVector::Zero();
					MoveToCursor.Timer = 0.f;

					MoveToCursor.Target = CommanderComp->GetCommandLocation();
					//// deferred:
					//Context.Defer().PushCommand<FMassDeferredChangeCompositionCommand>([&MoveToCursor, CommanderComp](FMassEntityManager& Manager){
					//	//NavigationSubsystem->EntityRequestNewPath(EntityHandle, PathFollowParams, InitialLocation, NewMoveToLocation, PathFragment);
					//	MoveToCursor.Target = CommanderComp->GetCommandLocation();
					//});
				}
				else
				{
					// update MoveTargetFragment
					FVector Direction = TargetLocation - CurrentLocation;
					Direction.Z = 0.f;
					Direction.Normalize();
					Velocity = Direction * Speed;
					Transform.SetRotation(Direction.ToOrientationQuat());
				}
				
			}
		}));
}

// Initializer (Observer)

UETW_MassMoveToCursorInitializer::UETW_MassMoveToCursorInitializer()
	: EntityQuery(*this)
{
	ObservedType = FMassMoveToCursorFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UETW_MassMoveToCursorInitializer::Register()
{
	check(ObservedType);
	UMassObserverRegistry::GetMutable().RegisterObserver(*ObservedType, Operation, GetClass());
}

void UETW_MassMoveToCursorInitializer::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMassMoveToCursorFragment>(EMassFragmentAccess::ReadWrite);
}

void UETW_MassMoveToCursorInitializer::Execute(FMassEntityManager& EntityManager,
	FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [World](FMassExecutionContext Context)
	{
		const TArrayView<FMassMoveToCursorFragment> MoveToCursorList = Context.GetMutableFragmentView<FMassMoveToCursorFragment>();

		TConstArrayView<FMassEntityHandle> Entities = Context.GetEntities();

		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			if (UMassCommanderComponent* CommanderComp = Cast<UMassCommanderComponent>(PlayerPawn->GetComponentByClass(UMassCommanderComponent::StaticClass())))
			{
				FMassMoveToCursorCommanderFragment CommanderFrag;
				CommanderFrag.CommanderComp = CommanderComp;

				for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
				{
					// this is actually not very good to create commander fragment copy for each entity
					Context.Defer().PushCommand<FMassCommandAddFragmentInstances>(Context.GetEntity(EntityIndex), CommanderFrag);

					FMassMoveToCursorFragment MoveToCursorFrag = MoveToCursorList[EntityIndex];
					MoveToCursorFrag.Timer = 0.f;
				}
			};
		}
	});

}