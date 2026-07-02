# Formats de jeux pris en charge

| Extension | Statut du parsing d'en-tête | Notes |
|---|---|---|
| `.iso` | ✅ Complet | Lecture directe de l'en-tête disque (Game ID @0x00, titre @0x20) |
| `.gcz` | ✅ Complet (en-tête brut) | Le conteneur compressé encapsule le même en-tête disque en clair au début du fichier |
| `.wbfs` | 🚧 Extension point | Nécessite de parcourir la table de partitions WBFS avant d'atteindre l'en-tête du disque |
| `.rvz` | 🚧 Extension point | Format compressé propre à Dolphin ; nécessite de lire l'en-tête RVZ dédié |

Voir `GameLibrary::parseGameFile()` dans `src/games/GameLibrary.cpp` pour le point d'extension.
