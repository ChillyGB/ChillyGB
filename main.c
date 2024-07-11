#include "raylib.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
#include "cartridge.h"
#include <stdio.h>
#include <string.h>

#define MIN(a, b) ((a)<(b)? (a) : (b))

int main(void) {
    // Initialize CPU, memory and timer
    cpu c = {};

    // Initialize Raylib
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(160*4, 144*4, "ChillyGB");
    SetWindowMinSize(160, 144);
    SetTargetFPS(60);
    Color pixels[144][160] = { 0 };
    for (int i = 0; i < 144; i++)
        for (int j = 0; j < 160; j++)
            pixels[i][j] = (Color){185, 237, 186, 255};
    Image display_image = {
            .data = pixels,
            .width = 160,
            .height = 144,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            .mipmaps = 1
    };
    Texture2D display = LoadTextureFromImage(display_image);

    char rom_name[256];

    // Initialize APU
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(512);
    load_audio_streams();

    bool rom_loaded = false;
    while(!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            if ((int) droppedFiles.count == 1) {
                rom_loaded = true;
                save_game(&c.cart, rom_name);
                strcpy(rom_name, droppedFiles.paths[0]);
                load_game(&c.cart, rom_name);
                initialize_cpu_memory(&c);
            }
            UnloadDroppedFiles(droppedFiles);
        }
        if (!rom_loaded) {
            float scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
            BeginDrawing();
                ClearBackground(BLACK);
                DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                               (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                            (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                            (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
                float fontsize = 7 * scale;
                int center = MeasureText("Drop a Game Boy ROM to start playing", fontsize);
                DrawText("Drop a Game Boy ROM to start playing", GetScreenWidth()/2 - center/2, GetScreenHeight()/2-fontsize/2, fontsize, (Color) {49, 106, 64, 255});
            EndDrawing();
        }
        else {
            execute(&c);
            Update_Audio(&c);

            if (video.draw_screen == true) {
                video.draw_screen = false;
                for (int i = 0; i < 144; i++) {
                    for (int j = 0; j < 160; j++) {
                        switch (video.display[i][j]) {
                            case 0:
                                pixels[i][j] = (Color) {185, 237, 186, 255};
                                break;
                            case 1:
                                pixels[i][j] = (Color) {118, 196, 123, 255};
                                break;
                            case 2:
                                pixels[i][j] = (Color) {49, 106, 64, 255};
                                break;
                            case 3:
                                pixels[i][j] = (Color) {10, 38, 16, 255};
                                break;
                        }
                    }
                }
                UpdateTexture(display, pixels);
                // Draw
                float scale = MIN((float) GetScreenWidth() / 160, (float) GetScreenHeight() / 144);
                BeginDrawing();
                    ClearBackground(BLACK);
                    DrawTexturePro(display, (Rectangle) {0.0f, 0.0f, (float) display.width, (float) display.height},
                                   (Rectangle) {(GetScreenWidth() - ((float) 160 * scale)) * 0.5f,
                                                (GetScreenHeight() - ((float) 144 * scale)) * 0.5f,
                                                (float) 160 * scale, (float) 144 * scale}, (Vector2) {0, 0}, 0.0f, WHITE);
                EndDrawing();
                uint16_t fps = GetFPS();
                char str[60];
                sprintf(str, "ChillyGB - %d FPS - %.1fx", fps, (float)(fps)/60);
                SetWindowTitle(str);
            }
        }
    }

    save_game(&c.cart, rom_name);
    CloseWindow();

    return 0;
}
