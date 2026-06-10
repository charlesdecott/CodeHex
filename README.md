# CodeHex

**Reconstruction procédurale du monde réel dans Unreal Engine 5, entièrement en C++.**

CodeHex génère à la volée un terrain explorable basé sur de vraies données géographiques : relief, imagerie aérienne et bâtiments sont chargés et maillés en temps réel autour du joueur, avec un système de LOD dynamique.

## 🎬 Démo

[![Démo CodeHex](https://img.youtube.com/vi/OgjPYFBnSik/maxresdefault.jpg)](https://youtu.be/OgjPYFBnSik)

▶️ **[Voir la vidéo de démonstration](https://youtu.be/OgjPYFBnSik)**

## 🛠️ Stack technique

- **[Unreal Engine 5](https://www.unrealengine.com/)** (5.4) — projet 100% **C++**
- **[RealtimeMeshComponent](https://github.com/TriAxis-Games/RealtimeMeshComponent)** — le plugin utilisé pour générer et mettre à jour les mesh en temps réel. C'est la pierre angulaire du projet : tous les mesh (terrain, bâtiments) sont construits au runtime via son API de `StreamSet` / `MeshBuilder`, bien plus performante que le `ProceduralMeshComponent` natif d'Unreal.
- **Données IGN** (open data) :
  - **RGE ALTI** — modèle numérique de terrain (altitudes)
  - **BD ORTHO** — orthophotographies aériennes (textures satellite)
  - **BDNB** — base de données nationale des bâtiments (empreintes au sol, hauteurs, altitudes)

## ⚙️ Comment on a fait

### 1. Découpage du monde en chunks (quadtree)

Le monde est géré par un actor `AEarth` ([Earth.cpp](Source/CodeHex/Private/Terrain/Earth.cpp)) qui orchestre une grille de chunks (`AChunk`). Chaque chunk est identifié par une clé `(X, Y, LOD)` :

- Chaque chunk contient **4 cellules** (sections bot_left, bot_right, top_left, top_right) — une structure de quadtree.
- À chaque niveau de LOD, la taille des cellules **double** (`n = 2`), jusqu'à `maxLOD` niveaux (8 par défaut).
- Chaque cellule est une grille de **101×101 vertices** dont les hauteurs viennent des données RGE ALTI.

### 2. Streaming dynamique autour du joueur

À chaque tick, `AEarth` compare la position du joueur aux chunks chargés :

- Les chunks manquants sont poussés dans une **file de génération** (`ChunksToGenerate`), triée par LOD pour charger le plus fin en premier, et traitée de façon **asynchrone** (`AsyncTask`) avec un nombre maximal de générations simultanées (`maxGeneratingChunks`).
- Les chunks trop éloignés sont détruits (`ClearChunks`).
- Quand un chunk fin apparaît, la section correspondante de son **chunk parent est masquée** (`HideShowParent`) via le système de visibilité par section de RealtimeMeshComponent — c'est ce qui permet la transition fluide entre les LODs sans trous ni doublons.

### 3. Génération des mesh avec RealtimeMeshComponent

Chaque cellule construit son mesh dans [Chunk.cpp](Source/CodeHex/Private/Terrain/Chunk.cpp) :

1. Les altitudes sont chargées depuis des **fichiers JSON pré-calculés** (un fichier par cellule et par LOD, issus du RGE ALTI).
2. Un `TRealtimeMeshBuilderLocal` génère vertices, triangles, UVs et tangentes dans un `FRealtimeMeshStreamSet`.
3. Le mesh est créé par section (`CreateSectionGroup`), ce qui permet de **montrer/cacher chaque cellule individuellement** lors des changements de LOD.
4. L'orthophoto correspondante (JPEG BD ORTHO) est chargée au runtime, convertie en `UTexture2D` et appliquée via une **material instance dynamique** (`M_Satelite`).

### 4. Bâtiments procéduraux

Au LOD le plus fin, les bâtiments sont générés depuis les empreintes BDNB :

1. Chaque empreinte (polygone 2D + altitude + hauteur) est lue depuis le JSON.
2. Le polygone est trié en sens horaire puis **triangulé par ear clipping** (implémentation maison dans `Triangulate`).
3. Le toit est extrudé à la hauteur du bâtiment et les **murs sont générés** en reliant l'empreinte au toit.
4. Une couleur de façade est tirée d'une **palette de 10 teintes réalistes** (avec variations aléatoires) et appliquée via une material instance dynamique (`M_Building`).

### 5. Édition dans l'éditeur

Le terrain peut être regénéré directement dans l'éditeur Unreal (sans lancer le jeu) grâce à `OnConstruction` et à un bouton `CallInEditor` sur l'actor `AEarth`, avec des coordonnées de départ (`X_id`, `Y_id`) modifiables pour choisir la zone du monde à charger.

### 🚧 En cours

- Génération des **routes** (WIP)

## 📁 Structure du projet

```
CodeHex/
├── CodeHex.uproject              # UE 5.4 + plugin RealtimeMeshComponent
├── Source/CodeHex/
│   ├── Public/Terrain/
│   │   ├── Earth.h               # Manager : streaming, LOD, files de génération
│   │   └── Chunk.h               # Chunk : mesh terrain, imagerie, bâtiments
│   └── Private/Terrain/
│       ├── Earth.cpp
│       └── Chunk.cpp
└── Content/
    ├── Terrain/                  # M_Satelite (imagerie), matériaux terrain
    ├── Buildings/                # M_Building (façades)
    └── Road/                     # Routes (WIP)
```

## 🚀 Lancer le projet

1. Cloner le repo (Git LFS requis pour les assets).
2. Installer **Unreal Engine 5.4** et le plugin **[RealtimeMeshComponent](https://github.com/TriAxis-Games/RealtimeMeshComponent)** (disponible sur le [Fab/Marketplace](https://www.fab.com/) ou en le plaçant dans `Plugins/`).
3. Générer les données (JSON d'altitudes RGE ALTI, tuiles BD ORTHO, JSON BDNB) et ajuster les chemins de données dans `Chunk.cpp`.
4. Clic droit sur `CodeHex.uproject` → *Generate Visual Studio project files*, compiler, puis ouvrir le projet.

## 🙏 Crédits

- [TriAxis-Games/RealtimeMeshComponent](https://github.com/TriAxis-Games/RealtimeMeshComponent) — le plugin de mesh temps réel qui rend tout ça possible
- [IGN](https://geoservices.ign.fr/) — RGE ALTI & BD ORTHO
- [BDNB](https://bdnb.io/) — base de données nationale des bâtiments
