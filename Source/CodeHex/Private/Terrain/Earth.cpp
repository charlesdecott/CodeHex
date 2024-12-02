// Fill out your copyright notice in the Description page of Project Settings.

#include <cmath>
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

    for (int inLOD = 1; inLOD <= maxLOD; inLOD++)
    {
        int current_xid = X_id - (X_id % static_cast<int>(std::pow(n, inLOD - 1)));
        int current_yid = Y_id - (Y_id % static_cast<int>(std::pow(n, inLOD - 1)));
        UE_LOG(LogTemp, Log, TEXT("AEarth :: LOD bef %d"), inLOD);
        Current_ids_LODs.Add(inLOD, FIntPoint(current_xid, current_yid));
        UE_LOG(LogTemp, Log, TEXT("AEarth :: LOD aft %d"), inLOD);
        SpawnNeighbors(current_xid, current_yid, inLOD);
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

            for (int inLOD = 1; inLOD <= maxLOD; inLOD++)
            {
                int new_xid = X_id+FMath::FloorToInt(PlayerLocation.X / (10000.0f));
                int new_yid = Y_id+FMath::FloorToInt(PlayerLocation.Y / (10000.0f));
                new_xid = new_xid - (new_xid % static_cast<int>(std::pow(n, inLOD - 1)));
                new_yid = new_yid - (new_yid % static_cast<int>(std::pow(n, inLOD - 1)));

                FIntPoint previous_coord = Current_ids_LODs[inLOD];
                int previous_xid = previous_coord.X;
                int previous_yid = previous_coord.Y;


                if (previous_xid != new_xid || previous_yid != new_yid)
                {
                    UE_LOG(LogTemp, Log, TEXT("AEarth :: Tick :: LOD change %d"), inLOD);
                    Current_ids_LODs[inLOD] = FIntPoint(new_xid, new_yid);

                    SpawnNeighbors(new_xid, new_yid, inLOD);
                    ClearChunks(new_xid, new_yid, inLOD);
                }
                else{
                    break;
                }
            }
        }
    }

    if(ChunksToGenerate.Num() > 0 && ChunksGenerating.Num() < 25)
    {
        if (ChunksToGenerate.IsValidIndex(0))
        {
            // Prendre la valeur de l'élément à l'index spécifié
            FIntVector Chunk = ChunksToGenerate[0];

            // Supprimer l'élément du tableau source
            ChunksToGenerate.RemoveAt(0);

            // Ajouter l'élément au tableau de destination
            ChunksGenerating.Add(Chunk);
            SpawnChunk(Chunk.X, Chunk.Y, Chunk.Z);
        }
    }
}


void AEarth::SpawnNeighbors(const int inX, const int inY, const int inLOD)
{
    for (int i = -neighbors; i <= neighbors; i++)
    {
        for (int j = -neighbors; j <= neighbors; j++)
        {
            FIntVector Chunk = FIntVector(inX + i*std::pow(n, inLOD - 1), inY + j*std::pow(n, inLOD - 1), inLOD);
            if(ChunksToGenerate.Contains(Chunk) || Chunks.Contains(Chunk) || ChunksGenerating.Contains(Chunk))
            {
                continue;
            }
            ChunksToGenerate.Add(Chunk);
            // SpawnChunk(inX + i*std::pow(n, inLOD - 1), inY + j*std::pow(n, inLOD - 1), inLOD);
        }
    }
    SortChunksToGenerate();
}

void AEarth::SortChunksToGenerate()
{
    ChunksToGenerate.Sort([](const FIntVector& A, const FIntVector& B)
    {
        return A.Z < B.Z;
    });
}

void AEarth::ClearChunks(const int inX, const int inY, const int inLOD)
{
    TArray<FIntVector> KeyToRemove;
    for (const FIntVector& Chunk : ChunksToGenerate)
    {
        if (Chunk.Z == inLOD)
        {
            if(
                Chunk.X - inX > neighbors*std::pow(n, inLOD - 1) || 
                Chunk.X - inX < -neighbors*std::pow(n, inLOD - 1) || 
                Chunk.Y - inY > neighbors*std::pow(n, inLOD - 1) || 
                Chunk.Y - inY < -neighbors*std::pow(n, inLOD - 1)
            )
            {
                KeyToRemove.Add(Chunk);
            }
        }
    }
    // Supprimer les chunks marqués pour suppression
    for (const FIntVector& Key : KeyToRemove)
    {
        ChunksToGenerate.Remove(Key);
    }


    KeyToRemove.Empty(); // Réinitialiser le tableau pour la prochaine utilisation
    TArray<FIntVector> KeyToRemove2;
    for (const TPair<FIntVector, AChunk*>& Elem : Chunks)
    {
            if (Elem.Key.Z == inLOD)
            {
                if(
                    Elem.Key.X - inX > neighbors*std::pow(n, inLOD - 1) || 
                    Elem.Key.X - inX < -neighbors*std::pow(n, inLOD - 1) || 
                    Elem.Key.Y - inY > neighbors*std::pow(n, inLOD - 1) || 
                    Elem.Key.Y - inY < -neighbors*std::pow(n, inLOD - 1)
                )
                {
                    KeyToRemove2.Add(Elem.Key);
                }

                // if (inLOD > 1)
                // {
                //     if(
                //         Chunks.Contains(FIntVector(inX, inY, inLOD-1)) &&
                //         Chunks.Contains(FIntVector(inX+static_cast<int>(std::pow(n, inLOD - 1)), inY, inLOD-1)) &&
                //         Chunks.Contains(FIntVector(inX, inY+static_cast<int>(std::pow(n, inLOD - 1)), inLOD-1)) &&
                //         Chunks.Contains(FIntVector(inX+static_cast<int>(std::pow(n, inLOD - 1)), inY+static_cast<int>(std::pow(n, inLOD - 1)), inLOD-1))
                //     )
                //     {
                //         KeyToRemove.Add(Elem.Key);
                //     }
                // }
            }
    }
    // Supprimez les chunks après avoir terminé l'itération
    for (const FIntVector& Key : KeyToRemove2)
    {
        if (AChunk* Chunk = Chunks[Key])
        {
            if (Chunk->Destroy())
            {
                Chunks.Remove(Key);
            }
        }
    }
}


void AEarth::SpawnChunk(const int inX, const int inY, const int inLOD)
{
    if (Chunks.Contains(FIntVector(inX, inY, inLOD)))
    {
        ChunksGenerating.Remove(FIntVector(inX, inY, inLOD));
        return;
    }

    // bool bHasLowerLODChunks = false;
    // if (inLOD > 1)
    // {
    //     int lowerLOD = inLOD - 1;
    //     int step = static_cast<int>(std::pow(n, lowerLOD));
    //     for (int i = 0; i < n; ++i)
    //     {
    //         for (int j = 0; j < n; ++j)
    //         {
    //             if (Chunks.Contains(FIntVector(inX + i * step, inY + j * step, lowerLOD)))
    //             {
    //                 bHasLowerLODChunks = true;
    //                 break;
    //             }
    //         }
    //         if (bHasLowerLODChunks)
    //         {
    //             break;
    //         }
    //     }
    // }

    // Exécuter la tâche de génération de chunk de manière asynchrone
    AsyncTask(ENamedThreads::GameThread, [this, inX, inY, inLOD]()
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(FVector((inX - X_id) * (10000.0f), (inY - Y_id) * (10000.0f), 0.0f));
        SpawnTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f)));
        AChunk* SpawnedChunk = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), SpawnTransform);
        Chunks.Add(FIntVector(inX, inY, inLOD), SpawnedChunk);

        // Utiliser AsyncTask pour revenir au thread du jeu pour la création de l'acteur
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, SpawnedChunk, inX, inY, inLOD]()
        {
            //RealtimeMeshes.Add(FIntVector(CurrentCellX + i*5, CurrentCellY + j*5, 1), RealtimeMesh);
            // SpawnedChunk->RealtimeMesh = RealtimeMesh;
            SpawnedChunk->X_id = inX;
            SpawnedChunk->Y_id = inY;
            SpawnedChunk->X_world = (inX - X_id) * 10000.0f;
            SpawnedChunk->Y_world = (inY - Y_id) * 10000.0f;
            SpawnedChunk->LOD = inLOD;
            SpawnedChunk->GenerateCells();

            if(false) // && Chunks.Contains(FIntVector(inX, inY, inLOD-1))
            {
                SpawnedChunk->SetCellsVisibility(false);
            }
            ChunksGenerating.Remove(FIntVector(inX, inY, inLOD));
        });
    });

}