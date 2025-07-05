# Map Times Widget - Documentation

## Vue d'ensemble

Cette fonctionnalité ajoute un widget qui affiche les **10 meilleurs temps** d'une carte DDNet en temps réel depuis l'API de https://www.ravenkog.com. Le widget est disponible dans un **menu dédié** accessible via une touche configurable.

## Fonctionnalités

- **Menu dédié fullscreen** : Menu plein écran centré avec fond semi-transparent
- **Affichage par maintien de touche** : Le menu s'affiche tant que la touche est maintenue enfoncée
- **Top 10** : Affiche les 10 meilleurs temps avec les noms des joueurs
- **Style visuel attrayant** : Différentes couleurs pour le podium (or, argent, bronze)
- **Commande console** : Affichage des top 10 dans le chat via la commande `show_top_10`
- **Configuration complète** : Touche configurable via le menu des binds standard
- **Taille de police configurable** : Variable `cl_map_times_text_size`

## Configuration

### Touche d'accès
- **Bind par défaut** : Touche `T`
- **Configuration** : Menu Settings > Controls > "Map Times"
- **Commande console** : `bind <key> +map_times`

### Variables de configuration
- `cl_map_times_text_size` : Taille du texte (10-200%, défaut: 50%)

### Commandes console
- `show_top_10` : Affiche le top 10 dans le chat
- `+map_times` : Commande bind pour ouvrir/fermer le menu (maintien de touche)

## Structure des fichiers

### Nouveaux fichiers créés :
- `src/game/client/components/maptimes.h` - Header du composant
- `src/game/client/components/maptimes.cpp` - Implémentation du composant
- `src/game/client/components/maptimes_menu.h` - Header du menu dédié
- `src/game/client/components/maptimes_menu.cpp` - Implémentation du menu dédié

### Fichiers modifiés :
- `src/game/client/gameclient.h` - Ajout du composant MapTimesMenu
- `src/game/client/gameclient.cpp` - Initialisation du composant et gestion input
- `src/engine/shared/config_variables.h` - Variables de configuration
- `src/game/client/components/menus_settings.cpp` - Ajout du bind dans les contrôles
- `src/game/client/components/binds.cpp` - Bind par défaut pour +map_times
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
- **Menu** : Settings → Appearance → HUD → "Show map times (top 10)"
- **Console** : `cl_showhud_map_times 1/0`

### Taille du texte configurable

Vous pouvez ajuster la taille du texte avec :
- **Console** : `cl_map_times_text_size <valeur>` (défaut: 50, min: 10, max: 200)
  - 50 = taille normale (50%)
  - 100 = taille double
  - 25 = taille très petite

## Utilisation

### Menu dédié Map Times
1. **Ouverture** : Maintenez la touche configurée (défaut: `T`)
2. **Affichage** : Le menu reste ouvert tant que la touche est maintenue
3. **Fermeture** : Relâchez la touche ou appuyez sur `ESC`

### Commande console
```
show_top_10
```

**Résultat dans le chat :**
```
=== Top 10 Records for dm1 ===
🥇 1. GreeN - 00:06:12.45
🥈 2. speedrunner - 00:06:14.23  
🥉 3. FastTee - 00:06:15.67
4. RocketPlayer - 00:06:18.91
5. QuickShot - 00:06:21.34
... (jusqu'au 10ème)
=========================
```

## Position et style

- **Menu style scoreboard** : Design inspiré du scoreboard avec layout structuré
- **Colonnes organisées** : Rang | Nom du joueur | Temps (alignement professionnel)
- **Titre contextuel** : "Map Times Leaderboard - [Nom de la carte]"
- **Couleurs podium** : 🥇 Or, 🥈 Argent, 🥉 Bronze pour le top 3
- **Lignes alternées** : Fond gris/noir alterné pour améliorer la lisibilité
- **Layout adaptatif** : Taille et espacement automatiques selon le nombre d'entrées
- **Taille de police** : Configurable via `cl_map_times_text_size` (défaut : 50%)
- **Format des temps** : HH:MM:SS.XX (2 décimales seulement)
- **Couleurs du texte** :
  - 1er place : Or (#FFD700)
  - 2ème place : Argent (#C0C0C0)
  - 3ème place : Bronze (#CD853F)
  - 4ème au 10ème : Blanc

## Fonctionnement technique

1. **Détection de changement de carte** : Le composant détecte automatiquement quand le joueur change de carte
2. **Requête HTTP** : Une requête est envoyée à l'API avec le nom de la carte
3. **Parsing JSON** : Les données JSON sont analysées pour extraire les top 10
4. **Mise en cache** : Les résultats sont mis en cache pour éviter les requêtes répétées
5. **Commande bind** : Le menu est contrôlé par le système de binds standard du client (+map_times)

## Améliorations récentes

### Version actuelle - Système de binds intégré
- **Bind standard** : Utilise le système de binds du client (+map_times)
- **Maintien de touche** : Le menu s'affiche tant que la touche est maintenue
- **Configuration via menu** : Touche configurable dans Settings > Controls > "Map Times"
- **Bind par défaut** : Touche T configurée automatiquement
- **Plus de variable custom** : Suppression de cl_map_times_key au profit du système standard
- **Top 10** : Affichage de 10 records (au lieu de 5)
- **Menu dédié fullscreen** : Interface moderne centrée
- **Commande console** : `show_top_10` pour afficher dans le chat
- **Taille configurable** : Variable `cl_map_times_text_size` (10-200%)

### Interface du menu dédié
- **Ouverture** : Maintenez la touche configurée (défaut: T)
- **Fermeture automatique** : Dès que vous relâchez la touche
- **Fermeture manuelle** : Touche ESC
- **Style** : Interface inspirée du scoreboard avec fond sombre et design structuré
- **Layout** : Titre en haut, colonnes organisées (Rang | Nom du joueur | Temps)
- **Couleurs podium** : Or (1er), Argent (2ème), Bronze (3ème)
- **Lignes alternées** : Fond gris alterné pour une meilleure lisibilité
- **Responsive** : Adaptation automatique de la taille selon le nombre d'entrées
- **Données temps réel** : Intégration directe avec l'API des Map Times
