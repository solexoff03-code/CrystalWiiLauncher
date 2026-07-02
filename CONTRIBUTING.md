# Contribuer à Crystal Wii Launcher

Merci de votre intérêt pour ce projet ! Toute contribution est bienvenue : code, thèmes, traductions, documentation, tests ou simples retours d'expérience.

## Avant de commencer

1. Vérifiez les [issues](../../issues) existantes pour éviter les doublons.
2. Pour une fonctionnalité importante, ouvrez d'abord une issue de discussion avant de coder.
3. Aucune ressource officielle de Nintendo (textures, sons, polices, icônes) ne doit jamais être ajoutée au dépôt. Tout artwork doit être original ou sous licence libre compatible.

## Mise en place de l'environnement

```powershell
git clone https://github.com/<votre-fork>/CrystalWiiLauncher.git
cd CrystalWiiLauncher
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Style de code

- **C++20**, formaté de façon cohérente avec le reste du fichier édité (indentation 4 espaces, accolades sur la même ligne).
- Un fichier = une responsabilité claire. Évitez les fonctions de plus d'une cinquantaine de lignes ; découpez en fonctions privées nommées explicitement.
- Commentez le *pourquoi*, pas le *quoi* évident.
- Les sous-systèmes (`core`, `games`, `discs`, `dolphin`, `controllers`, `ui`, `settings`, `updater`) ne doivent pas dépendre directement les uns des autres au-delà de ce qui existe déjà — passez par des signaux/slots Qt pour garder le couplage faible.
- Toute nouvelle dépendance externe doit être justifiée dans la pull request.

## Tests

Activez les tests avec `-DCWL_BUILD_TESTS=ON`. Les nouvelles fonctionnalités critiques (parsing de formats, logique de tri/recherche, etc.) doivent être accompagnées de tests quand c'est raisonnablement possible.

## Pull requests

1. Forkez le dépôt et créez une branche depuis `main` : `git checkout -b feature/ma-fonctionnalite`.
2. Committez avec des messages clairs (français ou anglais acceptés).
3. Assurez-vous que le projet compile (`cmake --build build`) avant de soumettre.
4. Décrivez clairement le changement, les captures d'écran étant très appréciées pour les changements d'UI.
5. Une review est nécessaire avant fusion.

## Signaler un bug

Merci d'inclure :
- Version de Windows et version de Crystal Wii Launcher
- Étapes de reproduction
- Le contenu pertinent de `logs/crystal-YYYY-MM-DD.log`
- Version de Dolphin utilisée, le cas échéant

## Code de conduite

Soyez respectueux et constructif. Ce projet est communautaire et n'est affilié ni à Nintendo ni à l'équipe Dolphin — merci de ne pas prétendre le contraire dans vos contributions ou communications.
