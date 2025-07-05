# Map Times Widget - Documentation

## Vue d'ensemble

Cette fonctionnalité ajoute un widget qui affiche les **10 meilleurs temps** d'une carte DDNet en temps réel depuis l'API de https://www.ravenkog.com. Le widget s'affiche **en haut à gauche** quand le **menu Tab (scoreboard)** est ouvert.

## Fonctionnalités

- **Affichage dans le menu Tab** : Récupère automatiquement les temps records depuis l'API lorsque vous ouvrez le scoreboard
- **Top 10** : Affiche les 10 meilleurs temps avec les noms des joueurs
- **Position optimisée** : Widget positionné en haut à gauche du menu Tab/scoreboard
- **Style visuel attrayant** : Différentes couleurs pour le podium (or, argent, bronze)
- **Configuration** : Peut être activé/désactivé via les paramètres HUD

## Structure des fichiers

### Nouveaux fichiers créés :
- `src/game/client/components/maptimes.h` - Header du composant
- `src/game/client/components/maptimes.cpp` - Implémentation du composant

### Fichiers modifiés :
- `src/game/client/components/hud.h` - Ajout de la déclaration RenderMapTimesHud()
- `src/game/client/components/hud.cpp` - Ajout de l'appel de rendu
- `src/game/client/gameclient.h` - Ajout du composant MapTimes
- `src/game/client/gameclient.cpp` - Initialisation du composant
- `src/engine/shared/config_variables.h` - Ajout de la variable ClShowhudMapTimes
- `src/game/client/components/menus_settings.cpp` - Option dans les paramètres
- `data/languages/french.txt` - Traduction française
- `CMakeLists.txt` - Ajout des fichiers sources

## API utilisée

L'API utilisée est : `https://www.ravenkog.com/api/maps?mapName={MAP_NAME}`

### Structure JSON de réponse :
```json
{
  "mapInfo": {
    "mapName": "Insanious",
    "points": 71,
    "difficulty": "Extreme",
    "mapCreator": "purrpT",
    "releaseDate": "2021-10-26"
  },
  "top100": [
    {
      "playerName": "定℘Ƭ͢Ʀ",
      "time": {"value": "00:20:02.440000"},
      "lastFinish": {"value": "2025-02-22T01:53:23.000Z"},
      "rank": 1
    }
    // ... autres records
  ]
}
```

## Configuration

La fonctionnalité peut être activée/désactivée via :
- **Menu** : Settings → Appearance → HUD → "Show map times (top 5)"
- **Console** : `cl_showhud_map_times 1/0`

## Position et style

- **Position** : Bas-droite de l'écran
- **Taille** : 180px × 120px
- **Arrière-plan** : Noir semi-transparent (60% d'opacité)
- **Coins arrondis** : 5px
- **Couleurs du texte** :
  - 1er place : Or (#FFD700)
  - 2ème place : Argent (#C0C0C0)
  - 3ème place : Bronze (#CD853F)
  - 4ème et 5ème : Blanc

## Fonctionnement technique

1. **Détection de changement de carte** : Le composant détecte automatiquement quand le joueur change de carte
2. **Requête HTTP** : Une requête est envoyée à l'API avec le nom de la carte
3. **Parsing JSON** : Les données JSON sont analysées pour extraire les top 5
4. **Mise en cache** : Les résultats sont mis en cache pour éviter les requêtes répétées
5. **Rendu** : Le widget est rendu par-dessus les autres éléments HUD

## Gestion d'erreurs

- **Cooldown** : 10 secondes entre les requêtes pour éviter le spam
- **Timeout** : 10 secondes de timeout pour les requêtes HTTP
- **Fallback** : Le widget ne s'affiche pas si aucune donnée n'est disponible
- **Logging** : Logs de débogage pour le troubleshooting

## Performance

- **Requêtes asynchrones** : N'bloque pas le jeu
- **Conteneurs de texte** : Optimisation du rendu via les text containers
- **Mise à jour conditionnelle** : Re-rendu uniquement si nécessaire

## Compatibilité

- Compatible avec toutes les cartes DDNet ayant des données sur l'API
- Fonctionne avec le système de configuration existant
- Respect du style visuel de TaterClient
