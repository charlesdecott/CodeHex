// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Chunk.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Earth.generated.h"

UCLASS()
class CODEHEX_API AEarth : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEarth();

	// Mesh
	TMap<FIntVector, AChunk *> Chunks;

	int32 X_id = 64590;
	int32 Y_id = 9140;

	int previous_xid = 0;
	int previous_yid = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
