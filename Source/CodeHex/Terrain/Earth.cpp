// Fill out your copyright notice in the Description page of Project Settings.

#include "Earth.h"

// Sets default values
AEarth::AEarth()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AEarth::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("AEarth :: Init"));

	int CurrentCellX = 64585;
	int CurrentCellY = 9135;

	for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            FTransform SpawnTransform;
            SpawnTransform.SetLocation(FVector(i*5 * 10000.0f, j*5 * 10000.0f, 0.0f));
            SpawnTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f)));

            AChunk *SpawnedChunk = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), SpawnTransform);
            Chunks.Add(FIntVector(CurrentCellX + i*5, CurrentCellY + j*5, 1), SpawnedChunk);
            //RealtimeMeshes.Add(FIntVector(CurrentCellX + i*5, CurrentCellY + j*5, 1), RealtimeMesh);
            // SpawnedChunk->RealtimeMesh = RealtimeMesh;
            SpawnedChunk->X_id = CurrentCellX + i*5;
            SpawnedChunk->Y_id = CurrentCellY + j*5;
            SpawnedChunk->X_world = i*5 * 10000.0f;
            SpawnedChunk->Y_world = j*5 * 10000.0f;
            SpawnedChunk->LOD = 1;
            SpawnedChunk->GenerateCells();
        }
    }
}

// Called every frame
void AEarth::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
