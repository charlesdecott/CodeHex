// Fill out your copyright notice in the Description page of Project Settings.

#include "Terrain/Earth.h"

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

	int CurrentCellX = 64584;
	int CurrentCellY = 9134;

	for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            FTransform SpawnTransform;
            SpawnTransform.SetLocation(FVector(i*2 * 10000.0f, j*2 * 10000.0f, 0.0f));
            SpawnTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f)));

            AChunk *SpawnedChunk = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), SpawnTransform);
            Chunks.Add(FIntVector(CurrentCellX + i*2, CurrentCellY + j*2, 1), SpawnedChunk);
            //RealtimeMeshes.Add(FIntVector(CurrentCellX + i*5, CurrentCellY + j*5, 1), RealtimeMesh);
            // SpawnedChunk->RealtimeMesh = RealtimeMesh;
            SpawnedChunk->X_id = CurrentCellX + i*2;
            SpawnedChunk->Y_id = CurrentCellY + j*2;
            SpawnedChunk->X_world = i*2 * 10000.0f;
            SpawnedChunk->Y_world = j*2 * 10000.0f;
            SpawnedChunk->LOD = 1;
            SpawnedChunk->GenerateCells();
        }
    }
}

// Called every frame
void AEarth::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        APawn* PlayerPawn = PlayerController->GetPawn();
        if (PlayerPawn)
        {
            FVector PlayerLocation = PlayerPawn->GetActorLocation();

            int32 x_chunk = FMath::FloorToInt(PlayerLocation.X / (2 * 10000.0f));
            int32 y_chunk = FMath::FloorToInt(PlayerLocation.Y / (2 * 10000.0f));


            if (previous_xid != x_chunk || previous_yid != y_chunk)
            {
                UE_LOG(LogTemp, Log, TEXT("AEarth :: Tick - Player Location: %s"), *PlayerLocation.ToString());
                UE_LOG(LogTemp, Log, TEXT("AEarth :: Tick - Player Location x : %d   y : %d"), x_chunk, y_chunk);
                UE_LOG(LogTemp, Log, TEXT("AEarth :: Tick - Player Location x chunk : %d   y chunk : %d"), X_id+x_chunk, Y_id+y_chunk);
            }
        }
    }
}

// Player Location: X=40001.982 Y=39999.782 Z=30914.047

// 40000 / 20000 = 2