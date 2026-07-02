# Crystal Wii Launcher

**Crystal Wii Launcher** est un launcher moderne, open source, pour Windows, inspiré de l'expérience du menu Wii — sans réutiliser aucune ressource officielle de Nintendo (textures, sons, icônes ou éléments graphiques). Il sert de frontend à [Dolphin Emulator](https://dolphin-emu.org/) : bibliothèque de jeux GameCube/Wii, détection de disques, gestion des manettes, et lancement en un clic.

> ⚠️ **Statut du projet** : squelette d'architecture fonctionnel (v0.1.0). Les subsystèmes principaux (scan de bibliothèque, détection de disques, intégration Dolphin, détection de manettes, mises à jour) sont implémentés et compilables ; certains points (parsing WBFS/RVZ complet, streaming HID Wiimote bas niveau, téléchargement de jaquettes) sont volontairement laissés comme extensions documentées — voir [Roadmap](#roadmap).

## Fonctionnalités

- 🎮 **Frontend Dolphin** : détection automatique de l'exécutable, lancement/fermeture de jeux, accès direct aux paramètres Dolphin.
- 💿 **Détection de disques** : scan des lecteurs CD/DVD, animation d'insertion, lecture de l'en-tête du disque pour identifier le jeu.
- 📚 **Bibliothèque de jeux** : scan asynchrone (ISO, WBFS, RVZ, GCZ), jaquettes, favoris, recherche, tri.
- 🕹️ **Manettes** : Wiimote (Bluetooth), Nunchuk, Classic Controller, MotionPlus, Balance Board, adaptateur GameCube USB, Xbox, DualSense, DualShock, clavier/souris — avec état de connexion en temps réel.
- 🎨 **Interface façon chaînes Wii** : grille animée, pointeur souris/Wiimote, transitions douces, plein écran/fenêtré, cible 60 FPS.
- ⚙️ **Paramètres complets** : thème, résolution, langue, audio, contrôleurs, chemins, options graphiques, mises à jour automatiques (GitHub Releases).
- 📝 **Journalisation** complète dans `logs/`.

## Architecture

```
CrystalWiiLauncher/
├── src/                 # Implémentations (.cpp), organisées par sous-système
│   ├── core/             # Logger, Settings
│   ├── games/             # GameLibrary (scan ISO/WBFS/RVZ/GCZ)
│   ├── discs/              # DiscScanner (Win32 optical drive polling)
│   ├── dolphin/             # DolphinManager (détection/lancement)
│   ├── controllers/          # ControllerManager (SDL2 + Bluetooth + libusb)
│   ├── ui/                    # MainWindow, ChannelGridWidget, WiiPointer
│   ├── settings/                # SettingsDialog
│   ├── updater/                   # UpdateManager (GitHub Releases)
│   └── main.cpp
├── include/              # En-têtes (.hpp), même organisation que src/
├── assets/                # Ressources embarquées (thèmes, icônes, fonts)
├── themes/                 # Thèmes utilisateur (JSON, voir docs/THEMING.md)
├── audio/                    # Effets sonores et musiques (originaux)
├── animations/                 # Ressources d'animation
├── controllers/                  # Profils de contrôleurs
├── discs/                          # (réservé — cache métadonnées disques)
├── dolphin/                          # Config liée à l'intégration Dolphin
├── cache/                              # Cache jaquettes/miniatures
├── saves/                                # Sauvegardes (ou lien vers le dossier Dolphin)
├── logs/                                   # Journaux d'exécution
├── settings/                                # config.json (généré au 1er lancement)
├── CMakeLists.txt
├── README.md
├── CONTRIBUTING.md
└── LICENSE
```

Chaque sous-système est découplé et communique via des signaux/slots Qt : `GameLibrary`, `DiscScanner`, `DolphinManager`, `ControllerManager` et `UpdateManager` n'ont aucune dépendance directe vers l'UI, ce qui les rend testables indépendamment et facilite le portage d'un futur frontend alternatif (QML, par exemple).

## Dépendances

| Dépendance | Rôle |
|---|---|
| C++20 / CMake ≥ 3.21 | Build system |
| Qt 6 (Core, Gui, Widgets, Network, Multimedia, Svg) | UI, réseau, audio |
| SDL2 | Détection manettes Xbox / DualSense / DualShock / génériques |
| OpenGL | Rendu accéléré des animations |
| libusb-1.0 *(optionnel)* | Adaptateur manette GameCube USB |
| Windows SDK (Bthprops, Setupapi) | Bluetooth Wiimote, énumération lecteurs optiques |
| nlohmann/json | Sérialisation des paramètres |

## Compilation (Windows, MSVC + vcpkg recommandé)

```powershell
# Installer les dépendances via vcpkg
vcpkg install sdl2 libusb nlohmann-json

# Configurer et compiler
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=<Qt6_install_dir>
cmake --build build --config Release
```

L'exécutable est généré dans `build/Release/CrystalWiiLauncher.exe`. Au premier lancement, l'application crée `settings/config.json` et détecte automatiquement une installation de Dolphin existante.

## Dolphin Emulator

Crystal Wii Launcher **ne redistribue pas** Dolphin : il détecte et pilote une installation locale existante, conformément à la licence GPLv2 de Dolphin. Téléchargez Dolphin sur [dolphin-emu.org](https://dolphin-emu.org/).

## Roadmap

- [ ] Parsing complet des en-têtes WBFS et RVZ (actuellement : ISO/GCZ)
- [ ] Backend HID bas niveau pour Wiimote (accéléromètre, IR, MotionPlus, Balance Board)
- [ ] Téléchargement automatique de jaquettes (GameTDB ou équivalent communautaire)
- [ ] Système de thèmes utilisateur additionnels
- [ ] Support Wiimote comme pointeur natif (actuellement : souris uniquement, l'API est prête)
- [ ] Tests unitaires (`CWL_BUILD_TESTS`)

## Licence

Distribué sous licence [MIT](LICENSE). Voir [CONTRIBUTING.md](CONTRIBUTING.md) pour contribuer.

## Avertissement

Ce projet n'est affilié à Nintendo ni à l'équipe Dolphin. Aucune ressource protégée (BIOS, IOS, ROM, texture ou son officiel) n'est incluse ou requise pour faire fonctionner le launcher — vous devez posséder légalement vos propres jeux et fichiers système.
