#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "SDL.h"
#include "SDL_ttf.h"
#include <string>

class DisplayManager {
public:
    DisplayManager();
    ~DisplayManager();

    bool init(const std::string& title, int width, int height);
    void clear();
    void present();
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    void handleEvents();
    bool isRunning() const;

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
};

#endif // DISPLAY_MANAGER_H