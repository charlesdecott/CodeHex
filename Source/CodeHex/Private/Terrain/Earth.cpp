// Fill out your copyright notice in the Description page of Project Settings.

#include <cmath>
#include "Terrain/Earth.h"

// Sets default values
AEarth::AEarth()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// when constructing the actor
void AEarth::OnConstruction(const FTransform &Transform)
{
	Super::OnConstruction(Transform);

    UE_LOG(LogTemp, Log, TEXT("AEarth :: Init"));

    MyCustomFunction();
}

void AEarth::MyCustomFunction()
{
    UE_LOG(LogTemp, Log, TEXT("MyCustomFunction called!"));

    // Ajoutez ici la logique que vous souhaitez exécuter lorsque le bouton est cliqué
    for (const TPair<FIntVector, AChunk*>& Elem : Chunks)
    {
            
        if (AChunk* Chunk = Chunks[Elem.Key])
        {
            if (Chunk->Destroy())
            {
                // Chunks.Remove(Elem.Key);
                continue;
            }
        }
    }
    Chunks.Empty();
    showHideQueue.Empty();
    ChunksGenerating.Empty();
    ChunksToGenerate.Empty();

    for (int inLOD = 1; inLOD <= maxLOD; inLOD++)
    {
        int current_xid = X_id - (X_id % static_cast<int>(std::pow(n, inLOD )));
        int current_yid = Y_id - (Y_id % static_cast<int>(std::pow(n, inLOD )));
        UE_LOG(LogTemp, Log, TEXT("AEarth :: LOD bef %d"), inLOD);
        Current_ids_LODs.Add(inLOD, FIntPoint(current_xid, current_yid));
        UE_LOG(LogTemp, Log, TEXT("AEarth :: LOD aft %d"), inLOD);
        SpawnNeighbors(current_xid, current_yid, inLOD);
    }


    for (int i = 0; i < maxGeneratingChunks; i++)
    {
        ProcessChunkQueue();
    }
}

// Called when the game starts or when spawned
void AEarth::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("AEarth :: BeginPlay"));

    MyCustomFunction();
}

// Called every frame
void AEarth::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    UpdateChunks();
    
}


void AEarth::ProcessChunkQueue()
{
    if(ChunksToGenerate.Num() > 0 && ChunksGenerating.Num() < maxGeneratingChunks)
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


void AEarth::ProcessHideShowQueue()
{
    if(showHideQueue.Num() > 0)
    {
        // TODO : Move to avoid during this operation every tick
        HideShowParent();
        showHideQueue.RemoveAt(0);
    }
}


void AEarth::UpdateChunks()
{
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
                new_xid = new_xid - (new_xid % static_cast<int>(std::pow(n, inLOD )));
                new_yid = new_yid - (new_yid % static_cast<int>(std::pow(n, inLOD )));

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

    ProcessChunkQueue();

    ProcessHideShowQueue();
}


void AEarth::SpawnNeighbors(const int inX, const int inY, const int inLOD)
{
    for (int i = -neighbors; i <= neighbors; i++)
    {
        for (int j = -neighbors; j <= neighbors; j++)
        {
            FIntVector Chunk = FIntVector(inX + i*std::pow(n, inLOD), inY + j*std::pow(n, inLOD), inLOD);
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
                Chunk.X - inX > neighbors*std::pow(n, inLOD ) || 
                Chunk.X - inX < -neighbors*std::pow(n, inLOD ) || 
                Chunk.Y - inY > neighbors*std::pow(n, inLOD ) || 
                Chunk.Y - inY < -neighbors*std::pow(n, inLOD )
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
                    Elem.Key.X - inX > neighbors*std::pow(n, inLOD ) || 
                    Elem.Key.X - inX < -neighbors*std::pow(n, inLOD ) || 
                    Elem.Key.Y - inY > neighbors*std::pow(n, inLOD ) || 
                    Elem.Key.Y - inY < -neighbors*std::pow(n, inLOD )
                )
                {
                    KeyToRemove2.Add(Elem.Key);
                }
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

// TODO : Avoid looping over all chunks => use last chunk coord to check only his parents
void AEarth::HideShowParent()
{

    for (int inLOD = 2; inLOD <= maxLOD; inLOD++)
    {
        for (const TPair<FIntVector, AChunk*>& Elem : Chunks)
        {
            if (Elem.Key.Z == inLOD)
            {
                // settings
                FIntVector Key = Elem.Key;
                AChunk* Chunk = Elem.Value;
                int loc_padding = static_cast<int>(std::pow(n, inLOD-1));

                // bot_left section
                FIntVector bot_left = FIntVector(Key.X, Key.Y, inLOD-1);
                int bot_left_section = 0;
                if(Chunks.Contains(bot_left))
                {
                    if(Chunk->GetCellVisibility(bot_left_section))
                    {
                        Chunk->SetCellVisibility(bot_left_section, false);
                    }
                }
                else
                {
                    if(!Chunk->GetCellVisibility(bot_left_section))
                    {
                        Chunk->SetCellVisibility(bot_left_section, true);
                    }
                }

                // bot_right section
                FIntVector bot_right = FIntVector(Key.X, Key.Y+loc_padding, inLOD-1);
                int bot_right_section = 1;
                if(Chunks.Contains(bot_right))
                {
                    if(Chunk->GetCellVisibility(bot_right_section))
                    {
                        Chunk->SetCellVisibility(bot_right_section, false);
                    }
                }
                else
                {
                    if(!Chunk->GetCellVisibility(bot_right_section))
                    {
                        Chunk->SetCellVisibility(bot_right_section, true);
                    }
                }

                // top_left section
                FIntVector top_left = FIntVector(Key.X+loc_padding, Key.Y, inLOD-1);
                int top_left_section = 2;
                if(Chunks.Contains(top_left))
                {
                    if(Chunk->GetCellVisibility(top_left_section))
                    {
                        Chunk->SetCellVisibility(top_left_section, false);
                    }
                }
                else
                {
                    if(!Chunk->GetCellVisibility(top_left_section))
                    {
                        Chunk->SetCellVisibility(top_left_section, true);
                    }
                }

                // top_right section
                FIntVector top_right = FIntVector(Key.X+loc_padding, Key.Y+loc_padding, inLOD-1);
                int top_right_section = 3;
                if(Chunks.Contains(top_right))
                {
                    if(Chunk->GetCellVisibility(top_right_section))
                    {
                        Chunk->SetCellVisibility(top_right_section, false);
                    }
                }
                else
                {
                    if(!Chunk->GetCellVisibility(top_right_section))
                    {
                        Chunk->SetCellVisibility(top_right_section, true);
                    }
                }
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

    // Exécuter la tâche de génération de chunk de manière asynchrone
    AsyncTask(ENamedThreads::GameThread, [this, inX, inY, inLOD]()
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(FVector((inX - X_id) * (10000.0f), (inY - Y_id) * (10000.0f), 0.0f));
        SpawnTransform.SetRotation(FQuat(FRotator(0.0f, 0.0f, 0.0f)));
        AChunk* SpawnedChunk = GetWorld()->SpawnActor<AChunk>(AChunk::StaticClass(), SpawnTransform);
        SpawnedChunk->SetFlags(RF_Transient); // Marquer l'acteur comme Transient
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

            // settings
            int loc_padding = static_cast<int>(std::pow(n, inLOD-1));

            // bot_left section
            FIntVector bot_left = FIntVector(inX, inY, inLOD-1);
            int bot_left_section = 0;
            if(Chunks.Contains(bot_left))
            {
                SpawnedChunk->SetCellVisibility(bot_left_section, false);
            }

            // bot_right section
            FIntVector bot_right = FIntVector(inX, inY+loc_padding, inLOD-1);
            int bot_right_section = 1;
            if(Chunks.Contains(bot_right))
            {
                SpawnedChunk->SetCellVisibility(bot_right_section, false);
            }

            // top_left section
            FIntVector top_left = FIntVector(inX+loc_padding, inY, inLOD-1);
            int top_left_section = 2;
            if(Chunks.Contains(top_left))
            {
                SpawnedChunk->SetCellVisibility(top_left_section, false);
            }

            // top_right section
            FIntVector top_right = FIntVector(inX+loc_padding, inY+loc_padding, inLOD-1);
            int top_right_section = 3;
            if(Chunks.Contains(top_right))
            {
                SpawnedChunk->SetCellVisibility(top_right_section, false);
            }

            ChunksGenerating.Remove(FIntVector(inX, inY, inLOD));
            ProcessChunkQueue();

            showHideQueue.Add(inLOD);
        });
    });

}
