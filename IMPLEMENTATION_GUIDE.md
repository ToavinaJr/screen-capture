# Guide d'Implémentation - Application de Screen Sharing Multiplateforme

## Architecture Complète

### 1. Composants Principaux

#### A. Couche Réseau (`src/network/`)
- **TLSConnection**: Gestion des connexions sécurisées SSL/TLS
- **StreamServer**: Serveur multi-clients avec gestion des connexions
- **Protocol**: Sérialisation/désérialisation des paquets

#### B. Couche Capture (`src/capture/`)
- **ScreenCapture**: Capture d'écran multiplateforme (X11 Linux, GDI+ Windows)
- **MicrophoneCapture**: Capture audio avec PortAudio
- **JPEGEncoder**: Compression des frames

#### C. Couche Affichage (`src/display/`)
- **SDLRenderer**: Affichage temps réel avec SDL2
- **DisplayManager**: Gestion des fenêtres et événements

#### D. Couche Threading (`src/threading/`)
- **ThreadPool**: Pool de workers pour traitement parallèle
- **SafeQueue**: Queue thread-safe pour communication inter-threads

#### E. Utilitaires (`src/utils/`)
- **Logger**: Système de logging multiplateforme
- **Config**: Gestion de la configuration

### 2. Flux de Données

```
SERVEUR:
Screen → Capture → JPEG Encode → TLS Encrypt → Network Send
  ↓
Microphone → Audio Buffer → Audio Frame → TLS Encrypt → Network Send

CLIENT:
Network Receive → TLS Decrypt → JPEG Decode → SDL Display
                              ↓
                         Audio Decode → Speaker Output
```

### 3. Multi-threading

```
Thread Principal: Gestion UI et événements
Thread Capture: Capture d'écran à 30 FPS
Thread Audio: Capture microphone continue
Thread Network: Envoi/réception des paquets
ThreadPool: Traitement des frames (encode/decode)
```

### 4. Protocole Réseau

#### Structure des Paquets
```
[Header 28 bytes][Payload N bytes]

Header:
- magic: 4 bytes (0x5343524E "SCRN")
- version: 1 byte
- packet_type: 1 byte
- flags: 2 bytes
- payload_size: 4 bytes
- sequence_number: 4 bytes
- timestamp: 8 bytes
```

#### Types de Paquets
1. HANDSHAKE: Négociation initiale
2. VIDEO_FRAME: Frame vidéo JPEG
3. AUDIO_FRAME: Frame audio PCM
4. CONFIG: Configuration (résolution, FPS, qualité)
5. HEARTBEAT: Keep-alive
6. DISCONNECT: Déconnexion gracieuse

### 5. Sécurité TLS/SSL

- Chiffrement AES-256-GCM
- Certificats auto-signés ou Let's Encrypt
- Authentification mutuelle optionnelle
- Vérification de l'intégrité des paquets

### 6. Optimisations

#### Performance
- Pool de threads pour encodage parallèle
- Queue lock-free pour réduction de contention
- Zero-copy buffers quand possible
- SIMD pour opérations sur pixels

#### Bande Passante
- Compression JPEG adaptative (qualité dynamique)
- Détection de régions changées (dirty rectangles)
- Skip frames si latence trop élevée
- Audio: compression Opus (optionnel)

### 7. Configuration CMake

#### Dépendances
```cmake
# Obligatoires
- SDL2 (affichage)
- Threads (multi-threading)
- JPEG (compression)

# Optionnelles
- OpenSSL (TLS)
- PortAudio (capture audio)
- X11 (Linux screen capture)
```

#### Builds
```bash
# Debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TLS=ON -DENABLE_AUDIO=ON
cmake --build build

# Windows avec vcpkg
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 8. Utilisation

#### Serveur
```bash
./server_app --port 9999 --fps 30 --quality 80 --enable-audio
```

#### Client
```bash
./client_app --server 192.168.1.100 --port 9999 --fullscreen
```

### 9. Tests

#### Tests Unitaires
- test_threading: ThreadPool et SafeQueue
- test_network: TLS et protocole
- test_audio: Capture microphone

#### Tests d'Intégration
- Latence end-to-end
- Bande passante sous charge
- Stabilité multi-clients

### 10. Roadmap

Phase 1 (Actuelle): Core fonctionnel
- ✅ Architecture multiplateforme
- ✅ Capture d'écran
- ✅ Réseau UDP/TCP
- ⏳ TLS/SSL
- ⏳ Affichage SDL2

Phase 2: Optimisations
- Audio streaming
- Multi-clients
- Compression adaptative
- Métriques de performance

Phase 3: Features avancées
- Contrôle à distance (clavier/souris)
- Partage fenêtre spécifique
- Enregistrement vidéo
- Interface web (WebRTC)

## Fichiers à Créer/Compléter

### Priorité Haute
1. src/network/TLSConnection.cpp - Implémentation TLS complète
2. src/display/SDLRenderer.cpp - Renderer SDL2 performant
3. src/audio/MicrophoneCapture.cpp - Capture audio PortAudio
4. src/network/StreamServer.cpp - Serveur multi-clients
5. src/core/Application.cpp - Application principale

### Priorité Moyenne
6. src/utils/Logger.cpp - Système de logging
7. src/threading/ThreadPool.cpp - Pool optimisé
8. tests/* - Tests unitaires

### Priorité Basse
9. src/capture/ScreenCapture.cpp - Capture multiplateforme
10. Documentation utilisateur
