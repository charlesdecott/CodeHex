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
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int maxGeneratingChunks = 5;


	// Mesh
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "YourCategory")
	TMap<FIntVector, AChunk *> Chunks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntVector> ChunksToGenerate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntVector> ChunksGenerating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int> showHideQueue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int X_id = 64584;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Y_id = 9134;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int, FIntPoint> Current_ids_LODs;

	// Fonction pour le bouton
    UFUNCTION(CallInEditor, Category = "YourCategory")
    void MyCustomFunction();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform &Transform) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SpawnChunk(const int inX, const int inY, const int inLOD);
	void SpawnNeighbors(const int inX, const int inY, const int inLOD);
	void SortChunksToGenerate();
    void ClearChunks(const int inX, const int inY, const int inLOD);
	void HideShowParent();

	void UpdateChunks();

	void ProcessChunkQueue();
	void ProcessHideShowQueue();
};
