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

	// Settings
	int n = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int neighbors = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int maxLOD = 8; // 10

	// Mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FIntVector, AChunk *> Chunks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntVector> ChunksToGenerate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntVector> ChunksGenerating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int> showHideQueue;

	int X_id = 64584;
	int Y_id = 9134;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int, FIntPoint> Current_ids_LODs;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SpawnChunk(const int inX, const int inY, const int inLOD);
	void SpawnNeighbors(const int inX, const int inY, const int inLOD);
	void SortChunksToGenerate();
    void ClearChunks(const int inX, const int inY, const int inLOD);
	void HideShowParent();
};
