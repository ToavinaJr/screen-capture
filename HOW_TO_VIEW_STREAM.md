# Comment Voir le Stream Vidéo

## Problème Actuel
Le `test_visual_viewer` a été créé mais il y a un problème de décodage des frames vidéo. Les données de `VideoFrame` dans le protocole ne sont pas des pixels bruts mais des données compressées.

## Solutions pour Voir le Stream

### Option 1: Utiliser test_viewer (statistiques uniquement)
```cmd
# Terminal 1 - Serveur
.\Release\screen_share.exe

# Terminal 2 - Client
.\Release\test_viewer.exe
```
**Affiche:** Statistiques en temps réel (FPS, frames reçues, bande passante)
**N'affiche PAS:** Les images vidéo

### Option 2: Modifier le Code pour Extraire les Pixels

Le problème est que:
1. `Application::captureFrame()` capture les pixels ARGB8888 bruts
2. Ces pixels sont mis dans `VideoFrame.data` 
3. Mais le viewer doit connaître le format exact pour les afficher

**Solution rapide:** Utiliser directement les données comme pixels ARGB8888

### Option 3: Affichage Côté Serveur (Ce qui fonctionne déjà)

L'application `screen_share.exe` **affiche déjà** la capture d'écran dans sa propre fenêtre SDL2!

```cmd
.\Release\screen_share.exe
```

Cette fenêtre montre:
- Le contenu capturé (rectangle animé, lignes colorées)
- C'est exactement ce qui est streamé aux clients
- Vous voyez en direct ce que les clients reçoivent

## Pour Ajouter l'Affichage Vidéo Côté Client

Il faut:
1. S'assurer que `VideoFrame.data` contient des pixels bruts ARGB8888
2. Utiliser `VideoFrame.width` et `VideoFrame.height` correctement  
3. Dans le viewer, créer une texture SDL avec ces dimensions
4. Copier les pixels directement dans la texture

## Utilisation Actuelle Recommandée

**Serveur avec GUI (voir la capture):**
```cmd
cd build\Release
.\screen_share.exe
```
- Ouvre une fenêtre 1280x720
- Affiche l'animation capturée
- Stream aux clients sur port 9999

**Client statistiques:**
```cmd
cd build\Release
.\test_viewer.exe
```
- Affiche FPS, frames reçues
- Confirme que le streaming fonctionne

**Test end-to-end complet:**
```cmd
cd build\Release
.\test_e2e_streaming.exe
```
- Lance serveur + client automatiquement
- Affiche taux de succès 100%

## Conclusion

Pour l'instant, la **meilleure façon de voir le stream vidéo** est d'observer la fenêtre de `screen_share.exe` côté serveur, qui montre exactement ce qui est capturé et streamé.

Le client `test_viewer.exe` confirme que les données arrivent correctement avec les statistiques.

Pour un viewer visuel complet, il faudrait implémenter un décodeur qui:
1. Lit les dimensions width/height de VideoFrame
2. Interprète VideoFrame.data comme pixels ARGB8888
3. Crée une texture SDL2 avec ces dimensions
4. Affiche la texture à l'écran
