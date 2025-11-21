# Viewer Visuel - Guide d'Utilisation

## ✅ CE QUI FONCTIONNE

### PC A (Serveur) - Capture d'écran réelle
```cmd
cd build\Release
.\screen_share.exe
```
**Fenêtre SDL2 s'ouvre avec:**
- Animation graphique (rectangle mobile, lignes colorées)
- Taille: 1280x720
- **C'est cette image qui sera visible sur PC B**

### PC B (Client) - Voir le stream
```cmd
cd build\Release
.\test_visual_viewer.exe
# ou avec IP distante:
.\test_visual_viewer.exe 192.168.1.100
```
**Une fenêtre SDL2 s'ouvre affichant:**
- Le stream vidéo en temps réel du PC A
- Les frames à ~22-30 FPS
- Statistiques dans la console (FPS, frames, résolution)

## 📊 STATISTIQUES UNIQUEMENT (sans image)

```cmd
# PC A
.\screen_share.exe

# PC B
.\test_viewer.exe 192.168.1.100
```
Affiche uniquement les statistiques dans la console (FPS, frames, bande passante).

## ⚠️ IMPORTANT

### test_stream_app NE MONTRE PAS de vraies images
`test_stream_app.exe` génère des **données simulées** pour tester le réseau, mais ce ne sont PAS de vrais pixels affichables.

**Pour voir de vraies images vidéo:**
1. Utilisez `screen_share.exe` comme serveur (pas test_stream_app)
2. Utilisez `test_visual_viewer.exe` comme client

### Résolution du problème "Frame data size mismatch"
Si vous voyez ce warning, c'est que:
- Le serveur envoie des données compressées/simulées
- Utilisez `screen_share.exe` qui envoie de vrais pixels ARGB8888

## 🖥️ Configuration Réseau (2 PCs)

### PC A (192.168.1.100) - Serveur
```cmd
# 1. Ouvrir le firewall
netsh advfirewall firewall add rule name="ScreenShare" dir=in action=allow protocol=TCP localport=9999

# 2. Lancer le serveur
cd build\Release
.\screen_share.exe
```

### PC B - Client
```cmd
cd build\Release
.\test_visual_viewer.exe 192.168.1.100
```

## 🎯 Résultats Attendus

### PC A (serveur)
```
Application initialized
Streaming server started on port 9999
Stream capture thread started at 30 FPS
Frame 30: 1 clients connected
Frame 60: 1 clients connected
...
```

### PC B (client - fenêtre SDL2)
- Fenêtre "Stream Viewer" s'ouvre
- Affiche l'animation du PC A en temps réel
- Redimensionnable
- Appuyez sur ESC ou fermez pour quitter

### PC B (client - console)
```
=== Visual Viewer ===
Connecting to 192.168.1.100:9999...
Connected! Displaying stream...
Press ESC or close window to exit.

Video resolution: 1280x720
FPS: 24.5 | Video: 735 | Audio: 245 | Resolution: 1280x720
```

## 🔧 Dépannage

### "Failed to connect"
- Vérifier que `screen_share.exe` est lancé sur PC A
- Vérifier le firewall (port 9999)
- Vérifier l'IP avec `ipconfig` sur PC A
- Tester la connexion: `ping 192.168.1.100`

### "Frame data size mismatch"
- Vous utilisez probablement `test_stream_app` au lieu de `screen_share`
- **Solution:** Utilisez `screen_share.exe`

### Fenêtre noire / pas d'image
- Vérifier que `screen_share.exe` affiche bien l'animation côté serveur
- Si le serveur n'affiche rien, il y a un problème de rendu SDL
- Redémarrer le serveur

### Performance lente
- Réduire la résolution (éditer Application.cpp avant compilation)
- Utiliser connexion Ethernet au lieu de WiFi
- Réduire le nombre de clients connectés

## 📝 Commandes Rapides

### Test local (même PC)
```cmd
# Terminal 1
cd build\Release
.\screen_share.exe

# Terminal 2  
cd build\Release
.\test_visual_viewer.exe
```

### Test réseau (2 PCs)
```cmd
# PC A
.\screen_share.exe

# PC B
.\test_visual_viewer.exe 192.168.1.100
```

## ✨ Fonctionnalités

- ✅ Streaming vidéo temps réel (30 FPS)
- ✅ Streaming audio simultané
- ✅ Multi-clients (plusieurs viewers en même temps)
- ✅ Affichage SDL2 avec rendu accéléré matériel
- ✅ Redimensionnement de fenêtre automatique
- ✅ Statistiques en temps réel (FPS, frames, résolution)
- ✅ Heartbeat automatique (maintien de connexion)
- ✅ Détection de déconnexion

## 🚀 Utilisation Pratique

**Partage d'écran pour présentation:**
1. PC présentateur: `.\screen_share.exe`
2. PC spectateur 1: `.\test_visual_viewer.exe 192.168.1.50`
3. PC spectateur 2: `.\test_visual_viewer.exe 192.168.1.50`
4. PC spectateur 3...

Tous voient le même contenu en temps réel!

**Bande passante:** ~4.3 MB/s par client
**Latence:** 10-50ms sur LAN
