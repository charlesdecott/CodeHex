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
    
	SpawnNeighbors(X_id, Y_id, 1);
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

            int x_chunk = FMath::FloorToInt(PlayerLocation.X / (2 * 10000.0f))*2;
            int y_chunk = FMath::FloorToInt(PlayerLocation.Y / (2 * 10000.0f))*2;


            if (previous_xid != X_id+x_chunk || previous_yid != Y_id+y_chunk)
            {
                previous_xid = X_id+x_chunk;
                previous_yid = Y_id+y_chunk;

                SpawnChunk(previous_xid, previous_yid, 1);
                SpawnNeighbors(previous_xid, previous_yid, 1);
                ClearChunks(previous_xid, previous_yid, 1);
            }
        }
    }
}

// Déclarez un compteur atomique pour suivre le nombre d'instances en cours d'exécution
std::atomic<int32> ActiveSpawnChunkTasks(0);

void AEarth::SpawnChunk(const int inX, const int inY, const int inLOD)
{
    if (Chunks.Contains(FIntVector(inX, inY, inLOD)))
    {
        return;
    }

    // Vérifiez si le nombre d'instances en cours d'exécution est inférieur à 3
    if (ActiveSpawnChunkTasks >= 25)
    {
        // Retarder l'exécution de cette tâche jusqu'à ce qu'une des tâches en cours soit terminée
        AsyncTask(ENamedThreads::GameThread, [this, inX, inY, inLOD]()
        {
            FPlatformProcess::Sleep(0.1f); // Attendre 100 ms avant de réessayer
            SpawnChunk(inX, inY, inLOD); // Réessayer de lancer la tâche
        });
        return;
    }

    // Incrémenter le compteur atomique
    ActiveSpawnChunkTasks++;

    // Exécuter la tâche de génération de chunk de manière asynchrone
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, inX, inY, inLOD]()
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(FVector((inX - X_id) * 10000.0f, (inY - Y_id) * 10000.0f, 0.0f));
        SpawnTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f)));

        // Utiliser AsyncTask pour revenir au thread du jeu pour la création de l'acteur
        AsyncTask(ENamedThreads::GameThread, [this, SpawnTransform, inX, inY, inLOD]()
        {
            AChunk* SpawnedChunk = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), SpawnTransform);
            Chunks.Add(FIntVector(inX, inY, inLOD), SpawnedChunk);
            //RealtimeMeshes.Add(FIntVector(CurrentCellX + i*5, CurrentCellY + j*5, 1), RealtimeMesh);
            // SpawnedChunk->RealtimeMesh = RealtimeMesh;
            SpawnedChunk->X_id = inX;
            SpawnedChunk->Y_id = inY;
            SpawnedChunk->X_world = (inX - X_id) * 10000.0f;
            SpawnedChunk->Y_world = (inY - Y_id) * 10000.0f;
            SpawnedChunk->LOD = inLOD;
            SpawnedChunk->GenerateCells();

            // Décrémenter le compteur atomique lorsque la tâche est terminée
            ActiveSpawnChunkTasks--;
        });
    });

}

void AEarth::SpawnNeighbors(const int inX, const int inY, const int inLOD)
{
    for (int i = -2; i < 3; i++)
    {
        for (int j = -2; j < 3; j++)
        {
            SpawnChunk(inX + i*2, inY + j*2, inLOD);
        }
    }
}

void AEarth::ClearChunks(const int inX, const int inY, const int inLOD)
{
    int distance = 8;
    TArray<FIntVector> KeyToRemove;
    for (const TPair<FIntVector, AChunk*>& Elem : Chunks)
    {
            if (Elem.Key.Z == 1)
            {
                if(
                    Elem.Key.X - previous_xid > distance || 
                    Elem.Key.X - previous_xid < -distance || 
                    Elem.Key.Y - previous_yid > distance || 
                    Elem.Key.Y - previous_yid < -distance
                )
                {
                    KeyToRemove.Add(Elem.Key);
                    UE_LOG(LogTemp, Log, TEXT("AEarth :: ClearChunks planned to x : %d   y : %d"), Elem.Key.X, Elem.Key.Y);
                }
            }
    }

    // Supprimez les chunks après avoir terminé l'itération
    for (const FIntVector& Key : KeyToRemove)
    {
        UE_LOG(LogTemp, Log, TEXT("AEarth :: ClearChunks x : %d   y : %d"), Key.X, Key.Y);
        if (AChunk* Chunk = Chunks[Key])
        {
            UE_LOG(LogTemp, Log, TEXT("AEarth :: ClearChunks in  x : %d   y : %d"), Key.X, Key.Y);
            if (Chunk->Destroy())
            {
                Chunks.Remove(Key);
            }
            UE_LOG(LogTemp, Log, TEXT("AEarth :: ClearChunks after  x : %d   y : %d"), Key.X, Key.Y);
        }
    }
}