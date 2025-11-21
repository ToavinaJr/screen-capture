#include "SDLRenderer.h"
#include "../utils/Logger.h"
#include <stdexcept>

SDLRenderer::SDLRenderer() : window(nullptr), renderer(nullptr) {}

SDLRenderer::~SDLRenderer() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

bool SDLRenderer::init(int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::string msg = "Failed to initialize SDL: " + std::string(SDL_GetError());
        Logger::log(Logger::LogLevel::WARN, msg);
        return false;
    }

    window = SDL_CreateWindow(
        "Screen Share",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::string msg = "Failed to create window: " + std::string(SDL_GetError());
        Logger::log(Logger::LogLevel::WARN, msg);
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::string msg = "Failed to create renderer: " + std::string(SDL_GetError());
        Logger::log(Logger::LogLevel::WARN, msg);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    std::string msg = "SDLRenderer initialized: " + std::to_string(width) + "x" + std::to_string(height);
    Logger::log(Logger::LogLevel::INFO, msg);
    return true;
}

void SDLRenderer::render() {
    // Placeholder - will be replaced with actual frame rendering
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
}

void SDLRenderer::clear() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void SDLRenderer::present() {
    SDL_RenderPresent(renderer);
}