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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FIntVector, AChunk *> Chunks;

	int X_id = 64584;
	int Y_id = 9134;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int previous_xid = 64584;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int previous_yid = 9134;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SpawnChunk(const int inX, const int inY, const int inLOD);
	void SpawnNeighbors(const int inX, const int inY, const int inLOD);
	void ClearChunks(const int inX, const int inY, const int inLOD);
};
