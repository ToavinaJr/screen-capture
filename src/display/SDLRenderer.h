#ifndef SDLRENDERER_H
#define SDLRENDERER_H

#include <SDL2/SDL.h>

class SDLRenderer {
public:
    SDLRenderer();
    ~SDLRenderer();

    bool init(int width, int height);
    void render();
    void clear();
    void present();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
};

#endif // SDLRENDERER_H