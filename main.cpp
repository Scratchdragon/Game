// C++ headers
#include <iostream>

// Raylib libraries
#include <raylib.h>

// Local headers
#include "src/player.cpp"
#include "src/tiles.cpp"
#include "src/world.cpp"

#include "src/random.h"

// Every season is 2 minutes
#define TIME_PER_CYCLE 600.0f

using namespace std;

float tile_scale = 0.5f;
float mouse_wheel_v = 0;

Vector2 movement_v = {0, 0};

void handle_input(_player * Player, float tile_w) {
    // Slow down
    movement_v.x /= 1.15;
    movement_v.y /= 1.15;

    // Basic movement keys
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        movement_v.y = -PLAYER_SPEED;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        movement_v.y = PLAYER_SPEED;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        movement_v.x = -PLAYER_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        movement_v.x = PLAYER_SPEED;
        
    // Update the positions
    Player->position.x += movement_v.x * (60 / (1/GetFrameTime()));
    Player->position.y += movement_v.y * (60 / (1/GetFrameTime()));

    // Check collisions
    Player->collision = false;
    for(int i = 0;i<tiles::overlays;++i) {
        if(dist(tiles::render_buffer_pos[i], {movement_v.x * (60 / (1/GetFrameTime())) - tile_w/2.0f, Player->size.y + tile_w/3.8f + movement_v.y * (60 / (1/GetFrameTime()))}) < tile_w/4)
            Player->collision = true;
    }
    if(Player->collision) {
        // Move back
        Player->position.x -= movement_v.x * (60 / (1/GetFrameTime()));
        Player->position.y -= movement_v.y * (60 / (1/GetFrameTime()));
    }
    
    // Mouse wheel    
    mouse_wheel_v += GetMouseWheelMove();
    if(mouse_wheel_v > 0 && tile_scale > 1.1)
        mouse_wheel_v = 0; 
    if(mouse_wheel_v < 0 && tile_scale < 0.2)
        mouse_wheel_v = 0;


    if(abs(mouse_wheel_v) <= 0.02) {
        if(mouse_wheel_v != 0)
            mouse_wheel_v = 0;
    }
    else {
        mouse_wheel_v = round(mouse_wheel_v * 75)/100;
        tile_scale += mouse_wheel_v/100;
    }
}

int main() {
    Random::init(time(0));

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
    world_map World = world_map();

    // Load tile textures
    tiles::load();

    // Rendering speed variables
    SetTargetFPS(60);
    float fps = 30;
    float avg_wr_ms = 5;
    float avg_ms = 5;
    clock_t wr_start, wr_end, render_start, render_end;

    // The size of a tile
    float tile_w = tiles::sprites[1].width * tile_scale;

    // The current season (there are 5)
    double season = 0.0f;

    // Visual effects
    Color tint = WHITE;

    // Main game loop
    while (!WindowShouldClose()) {
        tint = (Color){255 - (season*20), 255 - (season*30), 255 - (season*20), 255};
        if(season < 0.5f)
            tint = (Color){255 - ((5 - (season*9))*20), 255 - ((5 - (season*9))*30), 255 - ((5 - (season*9))*20), 255};

        // Update window variables
        window_size = {(float)GetRenderWidth(), (float)GetRenderHeight()};
        tile_w = tiles::sprites[1].width * tile_scale;

        render_start = clock();
        BeginDrawing();
            wr_start = clock();
    
            World.render(&Player, tile_w, tile_scale, tint);

            wr_end = clock();

            DrawText( to_string((int)fps).c_str(), 4, 4, 20, RAYWHITE );

            DrawTextureEx(dial_bg, {window_size.x-8-70, 8}, 0, 0.1, WHITE);
            DrawTexturePro(dial_needle, {0, 0, (float)dial_needle.width, (float)dial_needle.height}, {window_size.x-43, 42, 12, 60}, {6, 30}, season*72, WHITE);

        EndDrawing();
        
        // Update time data
        render_end = clock();
        fps = ((fps*30) + (1/GetFrameTime())) / 31;
        avg_wr_ms = ((avg_wr_ms*10) + (float(wr_end-wr_start) / float(CLOCKS_PER_SEC))) / 11;
        avg_ms = ((avg_ms*10) + (float(render_end-render_start) / float(CLOCKS_PER_SEC))) / 11;
        cout << "World render time = " << round(avg_wr_ms * 10000) / 10 << "ms : " << round(1/avg_wr_ms) << "\n";
        cout << "Total render time = " << round(avg_ms * 10000) / 10 << "ms : " << round(1/avg_ms) << "\n";

        // Update player
        Player.offset = sinf(GetTime()*8) * sqrt(pow(movement_v.x, 2) + pow(movement_v.y, 2));
        Player.size = {54 * tile_scale, 105 * tile_scale};

        // Handle the user input
        handle_input(&Player, tile_w);

        // Update the season
        season = dec_mod(GetTime()*(5.0f/TIME_PER_CYCLE), 5.0f);
    }

    // Unload everything
    Player.unload();
    CloseWindow();
    return 0;
}
