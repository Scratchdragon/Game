// C++ headers
#include <iostream>

// Raylib libraries
#include <raylib.h>

// Local headers
#include "src/player.cpp"
#include "src/tiles.cpp"
#include "src/world.cpp"

#include "src/random.h"

using namespace std;

bool debug = true;

float tile_scale = 2.0f;
float mouse_wheel_v = 0;

Vector2 movement_v = {0, 0};

void handle_input(_player * Player, float tile_w) {
    // Slow down
    movement_v.x /= 1.15;
    movement_v.y /= 1.15;

    // Basic movement keys
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        movement_v.y = PLAYER_SPEED;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        movement_v.y = -PLAYER_SPEED;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        movement_v.x = -PLAYER_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        movement_v.x = PLAYER_SPEED;
        
    // Update the positions
    Player->position.x += movement_v.x * (60 / (1/GetFrameTime()));
    Player->position.y += movement_v.y * (60 / (1/GetFrameTime()));

    if(Player->collision) {
        // Move back
        Player->position.x -= movement_v.x * (60 / (1/GetFrameTime()));
        Player->position.y -= movement_v.y * (60 / (1/GetFrameTime()));
    }
    
    // Mouse wheel
    mouse_wheel_v += GetMouseWheelMove()*tile_scale*4;
    if(mouse_wheel_v > 0 && tile_scale > 5 && !debug)
        mouse_wheel_v = 0; 
    if(mouse_wheel_v < 0 && tile_scale < 1 && !debug)
        mouse_wheel_v = 0;


    if(abs(mouse_wheel_v) <= 0.04) {
        if(mouse_wheel_v != 0)
            mouse_wheel_v = 0;
    }
    else {
        mouse_wheel_v = round(mouse_wheel_v * 8500)/10000;
        tile_scale += mouse_wheel_v/100;
    }
}

int main() {
    // Init window
    Vector2 base_size = {800, 500};
    Vector2 window_size = base_size;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(window_size.x, window_size.y, "Game");

    // Season dial
    Texture2D dial_bg = LoadTexture("resources/images/ui/season-dial.png");
    Texture2D dial_needle = LoadTexture("resources/images/ui/needle.png");

    // Init player
    _player Player = _player(&window_size);
    // The tiles are 50 wide
    Player.position = { WORLD_SIZE * 25, WORLD_SIZE * 25 }; // Middle of the map
    world_map World = world_map();
    World.generate_cave({WORLD_SIZE/2, WORLD_SIZE/2}, 7, 7);
    World.generate();

    // Load tile textures
    tiles::load();

    // Rendering speed variables
    SetTargetFPS(120);
    float fps = 30;
    float avg_wr_ms = 5;
    float avg_ms = 5;
    clock_t wr_start, wr_end, render_start, render_end;

    // The size of a tile
    float tile_w = tiles::sprites[1].width * tile_scale;

    // Main game loop
    while (!WindowShouldClose()) {
        // Update window variables
        if(window_size.x != GetRenderWidth() || window_size.y != GetRenderHeight()) {
            window_size = {(float)GetRenderWidth(), (float)GetRenderHeight()};
            tiles::shading_buffer = LoadRenderTexture(window_size.x, window_size.y);
        }
        
        tile_w = tiles::sprites[1].width * tile_scale;

        // Update the wall shading render texture
        BeginTextureMode(tiles::shading_buffer);
        ClearBackground((Color){0, 0, 0, 0});
        EndTextureMode();

        render_start = clock();
        BeginDrawing();
            ClearBackground(BLACK);

            wr_start = clock();
    
            World.render(&Player, tile_w, tile_scale, WHITE);

            wr_end = clock();

            Player.render();

            // Draw the wall shadows over the player to keep depth
            DrawTexture(tiles::shading_buffer.texture, 0, 0, WHITE);

            DrawText( to_string((int)fps).c_str(), 4, 4, 20, RAYWHITE);

        EndDrawing();
        
        // Update time data
        render_end = clock();
        fps = ((fps*30) + (1/GetFrameTime())) / 31;
        avg_wr_ms = ((avg_wr_ms*10) + (float(wr_end-wr_start) / float(CLOCKS_PER_SEC))) / 11;
        avg_ms = ((avg_ms*10) + (float(render_end-render_start) / float(CLOCKS_PER_SEC))) / 11;
        //cout << "World render time = " << round(avg_wr_ms * 10000) / 10 << "ms : " << round(1/avg_wr_ms) << "\n";
        //cout << "Total render time = " << round(avg_ms * 10000) / 10 << "ms : " << round(1/avg_ms) << "\n";

        // Update player
        Player.size = {16 * tile_scale, 16 * tile_scale};

        // Handle the user input
        handle_input(&Player, tile_w);
    }

    // Unload everything
    Player.unload();
    CloseWindow();
    return 0;
}
