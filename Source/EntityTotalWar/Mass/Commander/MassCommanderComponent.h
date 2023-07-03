﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MassCommanderComponent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct ENTITYTOTALWAR_API FMassCommanderCommandTrace
{
	GENERATED_BODY()

	//UPROPERTY()
	//FVector_NetQuantize Location;

	//UPROPERTY()
	//FVector_NetQuantizeNormal Normal;

	/** PrimitiveComponent hit by the trace. */
	//UPROPERTY()
	//TWeakObjectPtr<UPrimitiveComponent> ComponentHit;


	UPROPERTY(BlueprintReadWrite)
	FHitResult Trace;
};

//// All members of FHitResult are PODs.
//template<> struct TIsPODType<FMassCommanderCommandTrace> { enum { Value = true }; };
//
//template<>
//struct TStructOpsTypeTraits<FMassCommanderCommandTrace> : public TStructOpsTypeTraitsBase2<FMassCommanderCommandTrace>
//{
//	enum
//	{
//		WithNetSerializer = true,
//	};
//};


UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class ENTITYTOTALWAR_API UMassCommanderComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UMassCommanderComponent();

	// call this when input key action for command is received to trigger command
	UFUNCTION(BlueprintCallable)
	void ReceiveCommandInputAction();

	UFUNCTION(Server, Reliable)
	void ServerProcessInputAction(FVector_NetQuantize ClientCursorLocation, FVector_NetQuantizeNormal ClientCursorDirection, bool bTraceFromCursor);

	UFUNCTION(BlueprintImplementableEvent)
	void K2_ServerProcessInputAction();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_ReceiveCommandInputAction();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandProcessedDelegate, FMassCommanderCommandTrace, Result);
	
	UPROPERTY(BlueprintAssignable)
	FOnCommandProcessedDelegate OnCommandProcessedDelegate;

	UFUNCTION(BlueprintCallable)
	const FVector& GetCommandLocation() const { return CommandTraceResult.Trace.Location; }

	// try to find player camera component
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetTraceFromComponent(USceneComponent* InTraceFromComponent);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeComponent() override;

	UPROPERTY(EditDefaultsOnly)
	// by default is player camera component
	TWeakObjectPtr<USceneComponent> TraceFromComponent;

	UFUNCTION()
	void OnRep_CommandTraceResult();

	UPROPERTY(ReplicatedUsing = OnRep_CommandTraceResult, BlueprintReadOnly)
	FMassCommanderCommandTrace CommandTraceResult;

	
	bool RaycastCommandTarget(const FVector& ClientCursorLocation, const FVector& ClientCursorDirection, bool bTraceFromCursor = false);

	FHitResult LastCommandTraceResult;

	
};
