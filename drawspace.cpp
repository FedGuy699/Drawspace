#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int TOOLBAR_HEIGHT = 50;

enum Tool {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    TOOL_CLEAR,
    TOOL_EXPORT_PNG
};

struct Button {
    SDL_Rect rect;
    SDL_Color color;
    Tool action;
};

struct Line {
    float x1_relative;
    float y1_relative;
    float x2_relative;
    float y2_relative;
    SDL_Color color;
};

bool saveCanvasAsPNG(SDL_Renderer* renderer, int canvasWidth, int canvasHeight, const char* filename, const std::vector<Line>& lines) {
    SDL_Texture* targetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                                  SDL_TEXTUREACCESS_TARGET, canvasWidth, canvasHeight - TOOLBAR_HEIGHT);
    if (!targetTexture) {
        std::cerr << "Failed to create target texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderTarget(renderer, targetTexture);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    for (const auto& line : lines) {
        SDL_SetRenderDrawColor(renderer, line.color.r, line.color.g, line.color.b, line.color.a);
        int x1_pixel = static_cast<int>(line.x1_relative * canvasWidth);
        int y1_pixel = static_cast<int>(line.y1_relative * (canvasHeight - TOOLBAR_HEIGHT));
        int x2_pixel = static_cast<int>(line.x2_relative * canvasWidth);
        int y2_pixel = static_cast<int>(line.y2_relative * (canvasHeight - TOOLBAR_HEIGHT));
        SDL_RenderDrawLine(renderer, x1_pixel, y1_pixel, x2_pixel, y2_pixel);
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, canvasWidth, canvasHeight - TOOLBAR_HEIGHT, 
                                                        32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_DestroyTexture(targetTexture);
        return false;
    }

    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, 
                            surface->pixels, surface->pitch) != 0) {
        std::cerr << "Failed to read pixels: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_DestroyTexture(targetTexture);
        return false;
    }

    if (IMG_SavePNG(surface, filename) != 0) {
        std::cerr << "Failed to save PNG: " << IMG_GetError() << std::endl;
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_DestroyTexture(targetTexture);
        return false;
    }

    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_DestroyTexture(targetTexture);
    return true;
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL Initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() != 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "Initialization failed: " << TTF_GetError() << " / " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Drawspace", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
    if (!font) {
        std::cerr << "Font loading failed: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::vector<Button> buttons = {
        {{10, 10, 30, 30}, {0, 0, 0, 255}, COLOR_BLACK},
        {{50, 10, 30, 30}, {255, 0, 0, 255}, COLOR_RED},
        {{90, 10, 30, 30}, {0, 255, 0, 255}, COLOR_GREEN},
        {{130, 10, 30, 30}, {0, 0, 255, 255}, COLOR_BLUE},
        {{170, 10, 80, 30}, {128, 128, 128, 255}, TOOL_CLEAR},
        {{260, 10, 80, 30}, {200, 200, 0, 255}, TOOL_EXPORT_PNG}
    };

    SDL_Color currentColor = {0, 0, 0, 255};
    bool isDrawing = false;
    int lastX = 0, lastY = 0;
    SDL_Event e;
    std::vector<Line> drawnLines;

    bool running = true;
    int canvasWidth = WINDOW_WIDTH;
    int canvasHeight = WINDOW_HEIGHT - TOOLBAR_HEIGHT;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;

                if (mouseY < TOOLBAR_HEIGHT) {
                    for (auto& btn : buttons) {
                        SDL_Point point = {mouseX, mouseY};
                        if (SDL_PointInRect(&point, &btn.rect)) {
                            if (btn.action == TOOL_CLEAR) {
                                drawnLines.clear();
                                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                                SDL_RenderClear(renderer);
                            } 
                            else if (btn.action == TOOL_EXPORT_PNG) {
                                if (!saveCanvasAsPNG(renderer, canvasWidth, canvasHeight, "drawing.png", drawnLines)) {
                                    std::cerr << "Failed to save image" << std::endl;
                                }
                            } 
                            else {
                                currentColor = btn.color;
                            }
                        }
                    }
                } 
                else {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        isDrawing = true;
                        lastX = mouseX;
                        lastY = mouseY - TOOLBAR_HEIGHT;
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                isDrawing = false;
            }
            else if (e.type == SDL_MOUSEMOTION && isDrawing) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);

                if (mouseY > TOOLBAR_HEIGHT) {
                    mouseY -= TOOLBAR_HEIGHT;

                    float current_canvas_width = canvasWidth;
                    float current_canvas_height = canvasHeight;

                    drawnLines.push_back({
                        static_cast<float>(lastX) / current_canvas_width,
                        static_cast<float>(lastY) / current_canvas_height,
                        static_cast<float>(mouseX) / current_canvas_width,
                        static_cast<float>(mouseY) / current_canvas_height,
                        currentColor
                    });

                    lastX = mouseX;
                    lastY = mouseY;
                }
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                canvasWidth = e.window.data1;
                canvasHeight = e.window.data2 - TOOLBAR_HEIGHT;
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderClear(renderer);
                for (const auto& line : drawnLines) {
                    SDL_SetRenderDrawColor(renderer, line.color.r, line.color.g, line.color.b, line.color.a);
                    int x1_pixel = static_cast<int>(line.x1_relative * canvasWidth);
                    int y1_pixel = static_cast<int>(line.y1_relative * canvasHeight);
                    int x2_pixel = static_cast<int>(line.x2_relative * canvasWidth);
                    int y2_pixel = static_cast<int>(line.y2_relative * canvasHeight);
                    SDL_RenderDrawLine(renderer, x1_pixel, y1_pixel + TOOLBAR_HEIGHT, x2_pixel, y2_pixel + TOOLBAR_HEIGHT);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        for (const auto& line : drawnLines) {
            SDL_SetRenderDrawColor(renderer, line.color.r, line.color.g, line.color.b, line.color.a);
            int x1_pixel = static_cast<int>(line.x1_relative * canvasWidth);
            int y1_pixel = static_cast<int>(line.y1_relative * canvasHeight);
            int x2_pixel = static_cast<int>(line.x2_relative * canvasWidth);
            int y2_pixel = static_cast<int>(line.y2_relative * canvasHeight);
            SDL_RenderDrawLine(renderer, x1_pixel, y1_pixel + TOOLBAR_HEIGHT, x2_pixel, y2_pixel + TOOLBAR_HEIGHT);
        }

        for (const auto& btn : buttons) {
            SDL_SetRenderDrawColor(renderer, btn.color.r, btn.color.g, btn.color.b, btn.color.a);
            SDL_RenderFillRect(renderer, &btn.rect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &btn.rect);

            if (btn.action == TOOL_CLEAR || btn.action == TOOL_EXPORT_PNG) {
                const char* label = btn.action == TOOL_CLEAR ? "Clear" : "Save";
                SDL_Color textColor = {0, 0, 0, 255};
                SDL_Surface* textSurface = TTF_RenderText_Blended(font, label, textColor);
                if (textSurface) {
                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture) {
                        SDL_Rect textRect = {
                            btn.rect.x + (btn.rect.w - textSurface->w) / 2,
                            btn.rect.y + (btn.rect.h - textSurface->h) / 2,
                            textSurface->w,
                            textSurface->h
                        };
                        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                        SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
