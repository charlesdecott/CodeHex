// Fill out your copyright notice in the Description page of Project Settings.

#include "Terrain/Chunk.h"

// #include <vector>
// #include <cmath>
// #include <list>
#include <iostream>
#include <stdexcept>
#include <string>
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ImageUtils.h"

// Sets default values
AChunk::AChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	RealtimeMeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMeshComponent"));
    RootComponent = RealtimeMeshComponent;
}

// Called when the game starts or when spawned
void AChunk::BeginPlay()
{
	Super::BeginPlay();
}

void AChunk::OnConstruction(const FTransform &Transform)
{
	Super::OnConstruction(Transform);

	RealtimeMesh = RealtimeMeshComponent->InitializeRealtimeMesh<URealtimeMeshSimple>();
}

// Called every frame
void AChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AChunk::SetCellVisibility(int inSection, bool newVisibility)
{
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(FString::FromInt(inSection)));
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	RealtimeMesh->SetSectionVisibility(PolyGroup0SectionKey, newVisibility);
}

bool AChunk::GetCellVisibility(int inSection)
{
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(FString::FromInt(inSection)));
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	return RealtimeMesh->IsSectionVisible(PolyGroup0SectionKey);
}

void AChunk::SetCellsVisibility(bool newVisibility)
{
	for (int sec = 0; sec < 4; sec++)
	{
		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(FString::FromInt(sec)));
		const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
		RealtimeMesh->SetSectionVisibility(PolyGroup0SectionKey, newVisibility);
	}
}

void AChunk::GenerateCells()
{
	// settings
	int padding = std::pow(Cells_n, LOD - 1);

	// bot_left
	FIntPoint bot_left = FIntPoint(X_id, Y_id);
	int bot_left_section = 0;
	AddCell(bot_left.X, bot_left.Y, bot_left_section);

	// bot right
	FIntPoint bot_right = FIntPoint(X_id, Y_id+padding);
	int bot_right_section = 1;
	AddCell(bot_right.X, bot_right.Y, bot_right_section);

	// top_left
	FIntPoint top_left = FIntPoint(X_id+padding, Y_id);
	int top_left_section = 2;
	AddCell(top_left.X, top_left.Y, top_left_section);

	// bot right
	FIntPoint top_right = FIntPoint(X_id+padding, Y_id+padding);
	int top_right_section = 3;
	AddCell(top_right.X, top_right.Y, top_right_section);

	section = 4;
	// generate buildings
	if (LOD == 1){

		// setup material
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			FString MaterialPath = TEXT(
				"Material'/Game/Terrain/M_Building.M_Building'"); // Adjust path as needed
			UMaterialInterface *BaseMaterial =
				LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
			if (!BaseMaterial)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to load base material!"));
			}

			RealtimeMesh->SetupMaterialSlot(4, "BuildingMaterial", BaseMaterial); //nullptr
		});
	
		// Load buildings
		LoadBuildings(bot_left);
		LoadBuildings(bot_right);
		LoadBuildings(top_left);
		LoadBuildings(top_right);
		// Init
		// float AltitudeOffset = 21400.33f;
		// float BuildingHeight = 1000.0f;

		// TArray<FVector2D> loc_Vertices = {
		// 	FVector2D(0, 0),
		// 	FVector2D(1000, 0),
		// 	FVector2D(1000, 500),
		// 	FVector2D(500, 500),
		// 	FVector2D(500, 1000),
		// 	FVector2D(1000, 1000),
		// 	FVector2D(1000, 2000),
		// 	FVector2D(0, 2000)
		// };
		// GenBuilding(loc_Vertices, AltitudeOffset, BuildingHeight, 4);
	}
	
}

void AChunk::AddCell(const int ix, const int iy, const int inSection)
{
	if (Cells.Contains(FIntPoint(ix, iy)))
	{
		return;
	}
	Cells.Add(FIntPoint(ix, iy), inSection);

	// generate data
	TArray<TArray<float>> altitudes = LoadAltitudes(ix, iy);
	LoadImagery(ix, iy, inSection);

	// config
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(FString::FromInt(inSection))); // *FString::FromInt(LOD)
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	FRealtimeMeshSectionConfig SectionConfig;
	SectionConfig.MaterialSlot = inSection; //inSection

	// builder
	TSharedPtr<FRealtimeMeshStreamSet> StreamSet = MakeShared<FRealtimeMeshStreamSet>();

	TRealtimeMeshBuilderLocal<int32> Builder(*StreamSet.ToWeakPtr().Pin());

	Builder.EnableTexCoords();
	// Builder.EnableColors();
	Builder.EnableTangents();
	Builder.EnablePolyGroups();

	// init all values
	const int loc_Padding = std::pow(2, LOD - 1);
	const float loc_CellSize = loc_Padding * 100.0;

	// Vérifiez que Altitudes() n'est pas nul
	if (altitudes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Altitudes returned null. in AChunk::AddCell"));
		return;
	}

	// Generate Vertices and UVs
	for (int32 iVX = 0; iVX < Cell_Size; iVX++)
	{
		const TArray<float> &FloatArr = altitudes[iVX];

		for (int32 iVY = 0; iVY < Cell_Size; iVY++)
		{
			const float XX = (static_cast<float>(ix - X_id) / std::pow(2, LOD - 1) * (static_cast<float>(Cell_Size - 1) * loc_CellSize)) + static_cast<float>(iVX) * loc_CellSize;
			const float YY = (static_cast<float>(iy - Y_id) / std::pow(2, LOD - 1) * (static_cast<float>(Cell_Size - 1) * loc_CellSize)) + static_cast<float>(iVY) * loc_CellSize;

			const float ZZ = FloatArr[iVY] * 100;
			int32 V = Builder.AddVertex(FVector3f(XX, YY, ZZ)) // FloatArr[iVY] *
						  .SetTexCoord(FVector2f(
							  (iVX * loc_CellSize) / (Cell_Size * loc_CellSize),
							  (iVY * loc_CellSize) / (Cell_Size* loc_CellSize)));
			//   .SetNormalAndTangent(FVector3f(0.0f, 0.0f, 0.0f), FVector3f(0.0f, 0.0f, 0.0f))
			//   .SetColor(FColor::Blue);

			//DrawDebugPoint(GetWorld(), FVector(XX, YY, ZZ), 10.0f, FColor::Green, true, 1.0f);
			// .SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			// .SetColor(FColor::Red);

			if (iVX < Cell_Size - 1 && iVY < Cell_Size - 1)
			{
				int32 BaseIndex = iVY + iVX * Cell_Size;
				Builder.AddTriangle(
					V,
					V + 1,
					V + Cell_Size,
					0 /* This is the polygroup index */);
				Builder.AddTriangle(
					V + Cell_Size,
					V + 1,
					V + Cell_Size + 1,
					0 /* This is the polygroup index */);
			}

			// UE_LOG(LogTemp, Log, TEXT("AChunk :: AddCell debug pos :: V(%d) / x(%f) / y(%f)"), V, XX, YY);
		}
	}

	RealtimeMesh->CreateSectionGroup(GroupKey, *StreamSet);
	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, SectionConfig, LOD == 1 ? true : false);

	// if (inSection == 3)
	// {
	// 	RealtimeMesh->SetSectionVisibility(PolyGroup0SectionKey, false);
	// }

	// UE_LOG(LogTemp, Log, TEXT("AChunk :: AddCell finish :: x(%d) / y(%d) / section(%d) / lenAlti(%d)"), ix, iy, inSection, altitudes.Num());
	return;
}

TArray<TArray<float>> AChunk::LoadAltitudes(int inX_id, int inY_id)
{
	TArray<TArray<float>> Altitudes;
	const FString JsonFilePath = GetAltitudesJSONFilePath(inX_id, inY_id, LOD-1);
	try
	{
		// Load the file into a string
		const FString JsonString = LoadJSONFile(*JsonFilePath);

		// Parse the JSON
		Altitudes = ParseJSONAltitudes(JsonString);
	}
	catch (const std::exception &e)
	{
		UE_LOG(LogTemp, Error, TEXT("Erreur lors du parsing du fichier JSON : %s. Exception : %s"),
			   *JsonFilePath,
			   *FString(e.what()));
		Altitudes = Fallback_Generate_Altitudes();
	}

	return Altitudes;
}

void AChunk::LoadImagery(const int ix, const int iy, const int inSection)
{
	// Base du chemin
	const FString BasePath = FPaths::Combine(
		TEXT("C:/Users/forma/Documents/Mekivala/CodeHex/Data/BDORTHO/output/"),
		FString::Printf(TEXT("LOD%d"), LOD-1));

	// LOD File
	const FString Filepath = FString::Printf(TEXT("LOD%d_%d_%d.jpg"), LOD-1, ix, iy); //(static_cast<int>(Y_Chunk_id + iy * std::pow(5, LOD - 1))));

	// Construire le chemin complet
	const FString &FullPath = FPaths::Combine(BasePath, Filepath);

	// Load image file data into an array of bytes
	TArray<uint8> ImageData;
	if (!FFileHelper::LoadFileToArray(ImageData, *FullPath))
	{
		// UE_LOG(LogTemp, Error, TEXT("Failed to load image file from path: %s"),
		// 	   *FullPath);
	}

	AsyncTask(ENamedThreads::GameThread, [this, ImageData, inSection]()
    {
		// Convert the image data to a UTexture2D
		UTexture2D *LoadedTexture = FImageUtils::ImportBufferAsTexture2D(ImageData);
		if (!LoadedTexture)
		{
			// UE_LOG(LogTemp, Error, TEXT("Failed to convert image data to texture!"));
		}

		// Load a predefined material (ensure that you have an existing material in
		// the content directory)
		FString MaterialPath = TEXT(
			"Material'/Game/Terrain/M_Satelite.M_Satelite'"); // Adjust path as needed
		UMaterialInterface *BaseMaterial =
			LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (!BaseMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load base material!"));
		}

		// Create a dynamic material instance from the base material
		UMaterialInstanceDynamic *DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		if (!DynamicMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create dynamic material instance!"));
		}

		// Set the texture parameter in the material instance
		DynamicMaterial->SetTextureParameterValue(FName("Texture"), LoadedTexture);

		// Add Material to Imageries
		RealtimeMesh->SetupMaterialSlot(inSection, "ImageryMaterial", DynamicMaterial); //nullptr
	});

	return;
}

// Get the path of the chunk's JSON file
FString AChunk::GetAltitudesJSONFilePath(int inX_id, int inY_id, int inLOD)
{
	const FString BasePath = TEXT("C:/Users/forma/Documents/Mekivala/CodeHex/Data/RGEALTI/json/");
	const FString LODFolder = FString::Printf(TEXT("LOD%d"), inLOD);
	const FString LODFile = FString::Printf(TEXT("LOD%d_%d_%d.json"), inLOD, inX_id, inY_id);
	return FPaths::Combine(BasePath, LODFolder, LODFile);
}

// Load a json from a file
FString AChunk::LoadJSONFile(FString JsonFilePath)
{
	FString JsonString;
	try
	{
		FFileHelper::LoadFileToString(JsonString, *JsonFilePath);
	}
	catch (const std::exception &e)
	{
		UE_LOG(LogTemp, Error, TEXT("Impossible d'ouvrir le fichier JSON : %s. Exception : %s"),
			   *JsonFilePath,
			   *FString(e.what()));
		throw e;
	}
	return JsonString;
}

TArray<TArray<float>> AChunk::ParseJSONAltitudes(FString JsonString)
{
	TArray<TArray<float>> loc_Altitudes;
	try
	{
		// Lecture et parsing du JSON
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			throw std::runtime_error("Échec de la désérialisation du JSON.");
		}

		// Récupération et conversion des données
		for (const auto &RowValue : JsonObject->GetArrayField(TEXT("data")))
		{
			TArray<float> FloatRow;
			for (const auto &Value : RowValue->AsArray())
			{
				FloatRow.Add(Value->AsNumber());
			}
			loc_Altitudes.Add(FloatRow);
		}
	}
	catch (const std::exception &e)
	{
		UE_LOG(LogTemp, Error, TEXT("Erreur lors du ParseJSONAltitudes : Exception : %s"),
			   *FString(e.what()));
		throw e;
	}
	return loc_Altitudes;
}

// invoke this function if can't generate altitudes from JSON
TArray<TArray<float>> AChunk::Fallback_Generate_Altitudes()
{
	TArray<TArray<float>> loc_Altitudes;
	// Remplir AltitudeMatrix avec des FFloatArray
	for (int i = 0; i < Cell_Size; i++)
	{
		TArray<float> FloatRow;

		// Remplir chaque ligne avec 500 valeurs de 100.0f
		for (int j = 0; j < Cell_Size; j++)
		{
			FloatRow.Add(LOD * 100.0f);
		}

		// Ajouter la ligne � la matrice
		loc_Altitudes.Add(FloatRow);
	}
	return loc_Altitudes;
}




float CalculatePolygonArea(const TArray<FVector2D>& Vertices) {
    float Area = 0.0f;
    for (int i = 0; i < Vertices.Num(); ++i) {
        FVector2D Current = Vertices[i];
        FVector2D Next = Vertices[(i + 1) % Vertices.Num()];
        Area += (Current.X * Next.Y - Next.X * Current.Y);
    }
    return Area * 0.5f;
}

void EnsureClockwiseOrder(TArray<FVector2D>& Vertices) {
    if (CalculatePolygonArea(Vertices) > 0.0f) {
        Algo::Reverse(Vertices); // Inverser si nécessaire
    }
}


float CrossProduct(const FVector2D& V1, const FVector2D& V2) {
    return V1.X * V2.Y - V1.Y * V2.X;
}

bool IsPointInTriangle(FVector2D p, FVector2D a, FVector2D b, FVector2D c)
{
	FVector2D ab = b - a;
	FVector2D bc = c - b;
	FVector2D ca = a - c;

	FVector2D ap = p - a;
	FVector2D bp = p - b;
	FVector2D cp = p - c;

	float cross1 = CrossProduct(ab, ap);
	float cross2 = CrossProduct(bc, bp);
	float cross3 = CrossProduct(ca, cp);

	if(cross1 > 0.0f || cross2 > 0.0f || cross3 > 0.0f)
	{
		return false;
	}

	return true;
}


TArray<int> Triangulate(TArray<FVector2D>& Vertices) 
{
    TArray<int> Triangles;

    if (Vertices.Num() == 0) {
        throw std::invalid_argument("Vertices array is empty");
    }

    if (Vertices.Num() < 3) {
       	throw std::invalid_argument("Vertices.Num() < 3");
    }

    if (Vertices.Num() > 1024) {
       	throw std::invalid_argument("Vertices.Num() > 1024");
    }

	for (int i = 0; i < Vertices.Num(); ++i) 
	{
		for (int j = i + 1; j < Vertices.Num(); ++j) 
		{
			if (Vertices[i].Equals(Vertices[j]))
			{
				UE_LOG(LogTemp, Error, TEXT("Duplicate or near-identical points detected at index %d and %d  ::  iX(%f) / jX(%f)"), i, j, Vertices[i].X, Vertices[j].X);
				UE_LOG(LogTemp, Error, TEXT("Duplicate or near-identical points detected at index %d and %d  ::  iY(%f) / jY(%f)"), i, j, Vertices[i].Y, Vertices[j].Y);
			}
		}
	}

    TArray<int> IndexList;
	for (int i = 0; i < Vertices.Num(); i++)
	{
		IndexList.Add(i);
	}

	int MaxIterations = IndexList.Num() * IndexList.Num(); // Éviter une boucle infinie
	int IterationCount = 0;
    while (IndexList.Num() > 3) 
	{
		for (int i = 0; i < IndexList.Num(); ++i)
		{
            int A = IndexList[i];
            int B = IndexList[(i - 1 + IndexList.Num()) % IndexList.Num()];
            int C = IndexList[(i + 1) % IndexList.Num()];

            const FVector2D& VA = Vertices[A];
            const FVector2D& VB = Vertices[B];
            const FVector2D& VC = Vertices[C];

            FVector2D VAToVB = VB - VA;
            FVector2D VAToVC = VC - VA;

            // Test si le sommet est convexe
            if (CrossProduct(VAToVB, VAToVC) < 0.0f) {
                continue;
            }

            bool IsEar = true;

            // Vérifie si un autre sommet est dans le triangle
            for (size_t j = 0; j < Vertices.Num(); ++j) {
                if (j == A || j == B || j == C) {
                    continue;
                }

                const FVector2D& P = Vertices[j];
                if (IsPointInTriangle(P, VB, VA, VC)) {
                    IsEar = false;
                    break;
                }
            }

            if (IsEar) {
                Triangles.Add(B);
                Triangles.Add(A);
                Triangles.Add(C);

                IndexList.RemoveAt(i);
                break;
            }
        }

		if (++IterationCount > MaxIterations) {
			UE_LOG(LogTemp, Error, TEXT("Triangulate :: Triangulation failed: too many iterations."));
			return {};
		}
    }

    // Ajout du dernier triangle
    Triangles.Add(IndexList[0]);
    Triangles.Add(IndexList[1]);
    Triangles.Add(IndexList[2]);

    return Triangles;
}


void AChunk::GenBuilding(TArray<FVector2D> inVertices, float AltitudeOffset, float BuildingHeight, int inSection)
{
	EnsureClockwiseOrder(inVertices);

	TArray<FVector> Vertices;
	for (int i = 0; i < inVertices.Num(); ++i)
	{
        Vertices.Add(FVector(inVertices[i].X, inVertices[i].Y, AltitudeOffset));
    }

	TArray<int> Triangles = Triangulate(inVertices);

	if (Triangles.Num() == 0) {
        UE_LOG(LogTemp, Error, TEXT("Triangulation failed."));
        return;
    }

	// add roof
	int totalVertexCount = Vertices.Num();
	for (int i = 0; i < totalVertexCount; ++i)
	{
        Vertices.Add(FVector(Vertices[i].X, Vertices[i].Y, Vertices[i].Z + BuildingHeight));
    }


	int totalTriangleIndexCount = Triangles.Num();
	for (int i = 0; i < totalTriangleIndexCount; ++i)
	{
		Triangles.Add(Triangles[i]+totalVertexCount);
	}


	// add wall
	for (int i = 0; i < totalVertexCount; ++i)
	{
        Triangles.Add(i);
		Triangles.Add((i+1) % totalVertexCount);
		Triangles.Add((i+totalVertexCount) % (totalVertexCount*2));

		Triangles.Add((i+totalVertexCount) % (totalVertexCount*2));
		Triangles.Add((i+1) % totalVertexCount);
		if ((i+totalVertexCount+1) == (totalVertexCount*2))
		{
			Triangles.Add(totalVertexCount);
		}
		else{
			Triangles.Add((i+totalVertexCount+1) % (totalVertexCount*2));
		}
    }


	// GENERATE MESH
	// config
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(FString::FromInt(inSection))); // *FString::FromInt(LOD)
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	FRealtimeMeshSectionConfig SectionConfig;
	SectionConfig.MaterialSlot = 4; //inSection

	// builder
	TSharedPtr<FRealtimeMeshStreamSet> StreamSet = MakeShared<FRealtimeMeshStreamSet>();

	TRealtimeMeshBuilderLocal<int32> Builder(*StreamSet.ToWeakPtr().Pin());

	// Builder.EnableTexCoords();
	// Builder.EnableColors();
	Builder.EnableTangents();
	Builder.EnablePolyGroups();


	// Generate Vertices and UVs
	for (int32 V = 0; V < Vertices.Num(); V++)
	{
		int32 Vi = Builder.AddVertex(FVector3f(Vertices[V].X, Vertices[V].Y, Vertices[V].Z)); // FloatArr[iVY] *
	}

	for (int32 T = 0; T < Triangles.Num(); T = T + 3)
	{
		Builder.AddTriangle(
					Triangles[T],
					Triangles[T+1],
					Triangles[T+2],
					0 /* This is the polygroup index */);
	}


	RealtimeMesh->CreateSectionGroup(GroupKey, *StreamSet);
	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, SectionConfig, true);
}


void AChunk::LoadBuildings(FIntPoint chunk)
{
	// Path to the JSON file
	const FString FilePath = FPaths::Combine(
		TEXT("C:/Users/forma/Documents/Mekivala/CodeHex/Data/BDNB/output/"),
		FString::Printf(TEXT("LOD%d"), LOD-1),
		FString::Printf(TEXT("LOD%d_%d_%d.json"), LOD-1, chunk.X, chunk.Y));

    // Lire le contenu du fichier JSON
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Impossible de lire le fichier JSON : %s"), *FilePath);
        return;
    }

    // Analyser le contenu JSON
    TSharedPtr<FJsonValue> ParsedJson;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, ParsedJson) || !ParsedJson.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Erreur lors de l'analyse du JSON."));
        return;
    }

    // Vérifier si le JSON est un tableau
    if (ParsedJson->Type != EJson::Array)
    {
        UE_LOG(LogTemp, Error, TEXT("Le JSON racine n'est pas un tableau."));
        return;
    }

    // Parcourir le tableau JSON
    const TArray<TSharedPtr<FJsonValue>>& JsonArray = ParsedJson->AsArray();
    for (int32 Index = 0; Index < JsonArray.Num(); Index++)
    {
        const TSharedPtr<FJsonObject> JsonObject = JsonArray[Index]->AsObject();
        if (!JsonObject.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Élément %d non valide dans le JSON."), Index);
            continue;
        }

        // Exemple : Afficher des clés spécifiques
        FString BatimentID = JsonObject->GetStringField("batiment_construction_id");
        float Area = JsonObject->GetNumberField("area");
		float High = JsonObject->GetNumberField("high");
		float Altitude = JsonObject->GetNumberField("altitude");

        // Points (liste de coordonnées)
		TArray<FVector2D> loc_Vertices = {};
        const TArray<TSharedPtr<FJsonValue>>* PointsArray;
        if (JsonObject->TryGetArrayField("points", PointsArray))
        {
            for (const TSharedPtr<FJsonValue>& PointValue : *PointsArray)
            {
                const TArray<TSharedPtr<FJsonValue>>* PointCoords;
                if (PointValue->TryGetArray(PointCoords) && PointCoords->Num() == 2)
                {
                    float X = (*PointCoords)[0]->AsNumber();
                    float Y = (*PointCoords)[1]->AsNumber();
					
					float x_pad = static_cast<float>((chunk.X - X_id) / std::pow(2, LOD - 1)) * std::pow(2, LOD - 1) * 10000.0f;
					float y_pad = static_cast<float>((chunk.Y - Y_id) / std::pow(2, LOD - 1)) * std::pow(2, LOD - 1) * 10000.0f;

					loc_Vertices.Add(FVector2D(x_pad+X, y_pad+Y));
                }
            }
		
		GenBuilding(loc_Vertices, Altitude, High, section);

		section++;
        }
    }
}



// n=1 & MaxLOD=1 + triangulate ingame

// [2024.12.04-18.14.21:134][439]LogTemp: AEarth :: Init
// [2024.12.04-18.14.21:458][448]LogTemp: End




// n=1 & MaxLOD=1 + triangulate precomputed

// 
// 