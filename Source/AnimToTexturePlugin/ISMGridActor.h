// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "ISMGridActor.generated.h"

UCLASS()
class ANIMTOTEXTUREPLUGIN_API AISMGridActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AISMGridActor();

	UPROPERTY(EditAnywhere)
	int Rows;
	
	UPROPERTY(EditAnywhere)
	int Cols;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent;

	
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
