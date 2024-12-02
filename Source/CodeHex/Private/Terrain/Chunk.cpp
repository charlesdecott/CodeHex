// Fill out your copyright notice in the Description page of Project Settings.

#include "Terrain/Chunk.h"

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
	for (int ix = 0; ix < Cells_n; ix++)
	{
		for (int iy = 0; iy < Cells_n; iy++)
		{
			FIntPoint Key = FIntPoint(
				X_id + ix * std::pow(Cells_n, LOD - 1),
				Y_id + iy * std::pow(Cells_n, LOD - 1));

			TArray<TArray<float>> Altitudes = LoadAltitudes(Key.X, Key.Y);
			LoadImagery(Key.X, Key.Y, section);
			AddCell(Key.X, Key.Y, section, Altitudes);
			section++;
		}
	}
}
void AChunk::AddCell(const int ix, const int iy, const int inSection, TArray<TArray<float>> altitudes)
{
	if (Cells.Contains(FIntPoint(ix, iy)))
	{
		return;
	}
	Cells.Add(FIntPoint(ix, iy), inSection);

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
		TEXT("F:/MEKIVALA/theworld/BDORTHO/"),
		TEXT("data"),
		FString::Printf(TEXT("LOD%d"), LOD));

	// LOD File
	const FString Filepath = FString::Printf(TEXT("LOD%d_%d_%d.jpg"), LOD, ix, iy); //(static_cast<int>(Y_Chunk_id + iy * std::pow(5, LOD - 1))));

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
