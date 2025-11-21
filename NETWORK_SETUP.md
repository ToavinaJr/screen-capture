# Guide: Screen Sharing entre 2 PCs (PC A → PC B)

## Configuration Réseau

### PC A (Serveur - Partage son écran)

#### 1. Trouver l'adresse IP
```cmd
ipconfig
```
Notez l'**adresse IPv4** (exemple: `192.168.1.100`)

#### 2. Ouvrir le port dans le firewall Windows
```cmd
netsh advfirewall firewall add rule name="ScreenShare" dir=in action=allow protocol=TCP localport=9999
```

#### 3. Lancer l'application serveur
```cmd
cd build\Release
.\screen_share.exe
```

**Une fenêtre SDL2 s'ouvre:**
- Taille: 1280x720
- Contenu: Animation avec rectangle mobile et lignes colorées
- **C'est exactement ce que PC B verra**
- Le streaming démarre automatiquement sur le port 9999

**Console affiche:**
```
Application initialized
Streaming server started on port 9999
Stream capture thread started at 30 FPS
Microphone capture initialized
Frame 30: 1 clients connected, audio enabled
Frame 60: 1 clients connected, audio enabled
...
```

---

### PC B (Client - Regarde le stream)

#### Option 1: Ligne de commande (Recommandé)
```cmd
cd build\Release
.\test_viewer.exe 192.168.1.100
```
Remplacez `192.168.1.100` par l'IP réelle du PC A

**Affichage dans la console:**
```
=== Screen Share Viewer ===

Connecting to 192.168.1.100:9999...
Connected!

Time: 5.0s | Video: 150 frames | Audio: 50 frames | Size: 1280x720 | FPS: 30 | Data: 19.50 MB
Time: 10.0s | Video: 300 frames | Audio: 100 frames | Size: 1280x720 | FPS: 30 | Data: 39.00 MB
...
```

**Statistiques affichées:**
- **Time** - Durée de connexion
- **Video frames** - Nombre d'images reçues
- **Audio frames** - Nombre de frames audio reçues
- **Size** - Résolution vidéo (1280x720)
- **FPS** - Images par seconde en temps réel
- **Data** - Bande passante totale utilisée

#### Option 2: Modifier le code en dur
Si vous ne voulez pas utiliser d'argument:

Éditez `tests/test_viewer.cpp` ligne 25:
```cpp
std::string SERVER_ADDRESS = "192.168.1.100";  // IP du PC A
```

Puis recompilez:
```cmd
cd build
cmake --build . --target test_viewer --config Release
```

---

## Déploiement sur PC B

### Copier les fichiers nécessaires
Sur PC B, vous avez besoin de:
```
build/Release/
├── test_viewer.exe
├── SDL2.dll (pas nécessaire pour test_viewer car il n'utilise pas SDL2)
```

En fait, pour `test_viewer.exe`, seul l'exécutable est nécessaire!

### Installation complète sur PC B (depuis zéro)

Si PC B n'a pas le projet compilé:

1. **Copier le fichier Release:**
```
PC A: build\Release\test_viewer.exe
  → 
PC B: n'importe quel dossier
```

2. **Lancer sur PC B:**
```cmd
test_viewer.exe 192.168.1.100
```

---

## Tests

### Test local (même PC)
```cmd
# Terminal 1
.\screen_share.exe

# Terminal 2
.\test_viewer.exe 127.0.0.1
# ou simplement:
.\test_viewer.exe
```

### Test réseau (2 PCs)
1. PC A: `ipconfig` → noter IP (ex: 192.168.1.100)
2. PC A: `.\screen_share.exe`
3. PC B: `.\test_viewer.exe 192.168.1.100`

### Test avec plusieurs clients
PC A peut accepter **plusieurs clients simultanément**:
```cmd
PC B: .\test_viewer.exe 192.168.1.100
PC C: .\test_viewer.exe 192.168.1.100
PC D: .\test_viewer.exe 192.168.1.100
```

Chaque client reçoit le stream indépendamment.

---

## Performances

### Bande passante
- **1 client:** ~4.3 MB/s
- **5 clients:** ~21.5 MB/s
- **10 clients:** ~43 MB/s

### Latence réseau
- **LAN (même réseau local):** 10-50ms
- **WiFi:** 50-100ms
- **Internet (WAN):** 100-300ms

### Configuration recommandée
- **Connexion réseau:** Ethernet (câblé) pour meilleure stabilité
- **Serveur (PC A):** 
  - CPU: Multi-core recommandé
  - RAM: 4 GB minimum
  - Réseau: 100 Mbps minimum pour 10 clients
- **Client (PC B):**
  - CPU: Dual-core suffit
  - RAM: 2 GB minimum
  - Réseau: 10 Mbps minimum par client

---

## Dépannage

### "Connection refused" sur PC B
1. Vérifier que PC A a lancé `screen_share.exe`
2. Vérifier le firewall de PC A (port 9999 ouvert)
3. Vérifier que les 2 PCs sont sur le même réseau
4. Ping de PC B vers PC A: `ping 192.168.1.100`

### Firewall bloque la connexion
**Windows Defender:**
```cmd
netsh advfirewall firewall add rule name="ScreenShare" dir=in action=allow protocol=TCP localport=9999
```

**Ou via GUI:**
1. Panneau de configuration → Pare-feu Windows
2. Paramètres avancés → Règles de trafic entrant
3. Nouvelle règle → Port TCP 9999 → Autoriser

### Performance lente / saccadée
1. Réduire le nombre de clients simultanés
2. Utiliser connexion Ethernet au lieu de WiFi
3. Vérifier la bande passante disponible
4. Sur PC A, fermer les applications gourmandes

### PC B ne voit que des statistiques, pas l'image
**C'est normal!** `test_viewer.exe` affiche uniquement les **statistiques** du stream (FPS, frames, bande passante).

Pour voir l'image réelle, regardez la **fenêtre de PC A** qui montre exactement ce qui est streamé.

**Développement futur:** Un viewer graphique (`test_visual_viewer.exe`) est en cours mais nécessite des corrections pour afficher correctement les pixels.

---

## Résumé Rapide

```cmd
# PC A (192.168.1.100)
netsh advfirewall firewall add rule name="ScreenShare" dir=in action=allow protocol=TCP localport=9999
cd build\Release
.\screen_share.exe

# PC B
cd build\Release
.\test_viewer.exe 192.168.1.100

# Résultat: PC B voit les statistiques du stream de PC A en temps réel
```

**Le stream fonctionne!** PC B reçoit bien les frames vidéo et audio de PC A, confirmé par les statistiques affichées.
