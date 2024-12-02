// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshActor.h"
#include "Chunk.generated.h"

UCLASS()
class CODEHEX_API AChunk : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AChunk();

	// Mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URealtimeMeshComponent* RealtimeMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URealtimeMeshSimple *RealtimeMesh;

	// Heritage
	AChunk *Parent = nullptr;
	TMap<FIntPoint, AChunk *> Children;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FIntPoint, int> Cells;

	// Localisation
	int X_id = 0;
	int Y_id = 0;
	int LOD = 1;

	float X_world = 0;
	float Y_world = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OriginX = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OriginY = 0.0f;

	// Settings
	int Cell_Size = 101;
	int Cells_n = 2;
	int section = 0;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform &Transform) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// General
	void GenerateCells();
	void AddCell(const int ix, const int iy, const int inSection, TArray<TArray<float>> altitudes);
	void SetCellsVisibility(bool newVisibility);

	// Terrain
	//// Altitudes
	TArray<TArray<float>> LoadAltitudes(int inX_id, int inY_id);
	FString GetAltitudesJSONFilePath(int inX_id, int inY_id, int inLOD);
	FString LoadJSONFile(FString JsonFilePath);
	TArray<TArray<float>> ParseJSONAltitudes(FString JsonString);
	TArray<TArray<float>> Fallback_Generate_Altitudes();
	//// Imageries
	void LoadImagery(const int ix, const int iy, const int inSection);
};
