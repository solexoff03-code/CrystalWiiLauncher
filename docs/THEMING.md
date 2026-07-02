# Créer un thème

Les thèmes sont des fichiers JSON dans `themes/<nom-du-theme>/theme.json`, au même format que `assets/themes/default/theme.json`. Un thème définit :

- `colors` : palette utilisée par `ChannelGridWidget` et le reste de l'UI
- `fonts` : polices personnalisées (doivent être libres de droits)
- `sounds` : effets sonores (hover/select/back/disc_insert/startup)
- `animation` : paramètres de la grille de chaînes (colonnes, espacement, échelle au survol, durée de transition)

Aucun thème ne doit inclure de ressource Nintendo. Placez votre thème dans `themes/<nom>/` et sélectionnez-le depuis Paramètres → Général.
