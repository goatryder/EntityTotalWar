﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "MassCommanderComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"

void UMassCommanderComponent::SetTraceFromComponent_Implementation(USceneComponent* InTraceFromComponent)
{
	if (IsValid(InTraceFromComponent) && TraceFromComponent == nullptr)
	{
		TraceFromComponent = InTraceFromComponent;
	}
}

bool UMassCommanderComponent::RaycastCommandTarget()
{
	bool bSuccessHit = false;

	bool bTraceFromCursor = false;
	APlayerController* PC = nullptr;
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PlayerPC = OwnerPawn->GetController<APlayerController>())
		{
			PC = PlayerPC;
			bTraceFromCursor = PC->bShowMouseCursor;
		}
	}

	if (bTraceFromCursor && PC)
	{
		FVector TraceStart;
		FVector TraceDirection;
		PC->DeprojectMousePositionToWorld(TraceStart, TraceDirection);
		const FVector TraceEnd = TraceStart + TraceDirection * 30000.f;

		bSuccessHit = GetWorld()->LineTraceSingleByChannel(LastCommandTraceResult, TraceStart, TraceEnd, ECC_Visibility);
	}
	else if (TraceFromComponent.IsValid())
	{
		const FVector TraceStart = TraceFromComponent->GetComponentLocation();
		const FVector TraceDirection = TraceFromComponent->GetForwardVector();
		const FVector TraceEnd = TraceStart + TraceDirection * 30000.f;

		bSuccessHit = GetWorld()->LineTraceSingleByChannel(LastCommandTraceResult, TraceStart, TraceEnd, ECC_Visibility);

		//CommandTraceResult = { LastCommandTraceResult.Location, LastCommandTraceResult.Normal, LastCommandTraceResult.Component };
		CommandTraceResult = { LastCommandTraceResult };
	}
	else
	{
		UE_DEBUG_BREAK();
	}

	return bSuccessHit;
}

UMassCommanderComponent::UMassCommanderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMassCommanderComponent::ReceiveCommandInputAction()
{
	K2_ReceiveCommandInputAction();

	ServerProcessInputAction();
}

void UMassCommanderComponent::ServerProcessInputAction_Implementation()
{
	RaycastCommandTarget();
	K2_ServerProcessInputAction();
	OnCommandProcessedDelegate.Broadcast(CommandTraceResult);
}

void UMassCommanderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CommandTraceResult);
}

void UMassCommanderComponent::InitializeComponent()
{
	Super::InitializeComponent();
	SetTraceFromComponent(GetOwner()->GetComponentByClass<UCameraComponent>());
}

void UMassCommanderComponent::OnRep_CommandTraceResult()
{
	OnCommandProcessedDelegate.Broadcast(CommandTraceResult);
}
