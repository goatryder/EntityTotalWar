// Fill out your copyright notice in the Description page of Project Settings.

#include "ETW_MassSquadUnitsReplicator.h"
#include "MassReplicationTransformHandlers.h"
#include "ETW_MassSquadUnitsReplicatedAgent.h"
#include "ETW_MassSquadUnitsBubble.h"


void UETW_MassSquadUnitsReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
	FMassReplicationProcessorPositionYawHandler::AddRequirements(EntityQuery);
	FMassReplicationProcessorSquadUnitHandler::AddRequirements(EntityQuery);
}

void UETW_MassSquadUnitsReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

	FMassReplicationProcessorPositionYawHandler PositionYawHandler;
	FMassReplicationProcessorSquadUnitHandler SquadUnitHandler;
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;

	auto CacheViewsCallback = [&RepSharedFrag, &PositionYawHandler, &SquadUnitHandler](FMassExecutionContext& Context)
	{
		PositionYawHandler.CacheFragmentViews(Context);
		SquadUnitHandler.CacheFragmentViews(Context);
		RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		check(RepSharedFrag);
	};

	auto AddEntityCallback = [&RepSharedFrag, &PositionYawHandler, &SquadUnitHandler](FMassExecutionContext& Context, const int32 EntityIdx, FETW_MassReplicatedSquadUnitsAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)->FMassReplicatedAgentHandle
	{
		AETW_MassSquadUnitClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AETW_MassSquadUnitClientBubbleInfo>(ClientHandle);

		PositionYawHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedPositionYawDataMutable());
		SquadUnitHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedSquadUnitDataMutable());

		return BubbleInfo.GetSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
	};

	auto ModifyEntityCallback = [&RepSharedFrag, &PositionYawHandler, &SquadUnitHandler](FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type LOD, const float Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AETW_MassSquadUnitClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AETW_MassSquadUnitClientBubbleInfo>(ClientHandle);

		FETW_MassSquadUnitsClientBubbleHandler& Bubble = BubbleInfo.GetSerializer().Bubble;

		const bool bLastClient = RepSharedFrag->CachedClientHandles.Last() == ClientHandle;

		PositionYawHandler.ModifyEntity<FETW_MassSquadUnitsFastArrayItem>(Handle, EntityIdx, Bubble.GetTransformHandlerMutable());
		SquadUnitHandler.ModifyEntity(Handle, EntityIdx, Bubble.GetSquadUnitsHandlerMutable(), bLastClient);
	};

	auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AETW_MassSquadUnitClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AETW_MassSquadUnitClientBubbleInfo>(ClientHandle);

		BubbleInfo.GetSerializer().Bubble.RemoveAgentChecked(Handle);
	};

	CalculateClientReplication<FETW_MassSquadUnitsFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);

#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}



