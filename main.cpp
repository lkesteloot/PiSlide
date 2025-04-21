
#include <stdio.h>
#include <iostream>
#include <stdexcept>

#include "raylib.h"
#include "raymath.h"

#include "database.h"

int main_can_throw() {
    Database database;
    database.printPersons();
    return 0;
    // SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(0, 0, "Hello Raylib");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    printf("Screen size: %d %d\n", screenWidth, screenHeight); // 1728 1117

    SetTargetFPS(30);

    Texture texture1 = LoadTexture("resources/photo1.jpg");
    Texture texture2 = LoadTexture("resources/photo2.jpg");

    float t = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Hello Raylib", 200, 200, 20, WHITE);
        {
            float scale = static_cast<float>(screenWidth)/texture1.width*Lerp(0.8, 1.2, t);
            float x = (screenWidth - texture1.width*scale)/2;
            float y = (screenHeight - texture1.height*scale)/2;
            DrawTextureEx(texture1, Vector2 { x, y }, 0, scale, Fade(WHITE, t));
        }
        {
            float scale = static_cast<float>(screenWidth)/texture2.width*Lerp(1.2, 0.8, t);
            float x = (screenWidth - texture2.width*scale)/2;
            float y = (screenHeight - texture2.height*scale)/2;
            DrawTextureEx(texture2, Vector2 { x, y }, 0, scale, Fade(WHITE, 1 - t));
        }
        t += 0.003;
        if (t > 1) {
            t = 1;
        }

        DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadTexture(texture1);
    UnloadTexture(texture2);

    CloseWindow();

    return 0;
}

int main() {
    try {
        return main_can_throw();
    } catch (const std::exception &e) {
        std::cerr << "Caught exception (" << e.what() << ")" << std::endl;
    }
    return -1;
}
