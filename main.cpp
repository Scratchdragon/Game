// C++ headers
#include <iostream>

// Raylib libraries
#include <raylib.h>

// Local headers
#include "src/tiles.cpp"
#include "src/world.cpp"
#include "src/player.cpp"

#include "src/random.h"

using namespace std;

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

#define DEBUG true
#define TPS 10

float tile_scale = 2.0f;

// Global variables for input
Vector2 movement_v = {0, 0};
float mouse_wheel_v = 0;

// Global world variables
world_map World;
long long ticks; // The ticks elapsed since game start

bool check_collision(_player * Player) {
    Player->collision = 
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 1.4)}).id) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 0.6)}).id) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 1.4)}).id) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 0.6)}).id);
    return Player->collision;
}

void handle_input(_player * Player, float tile_w, Vector2 center) {
    // Slow down
    movement_v.x /= 1.15;
    movement_v.y /= 1.15;

    // Basic movement keys
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        movement_v.x = sin(Player->rotation/(180/PI)) * PLAYER_SPEED;
        movement_v.y = cos(Player->rotation/(180/PI)) * PLAYER_SPEED;
        if(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
            movement_v.x = -movement_v.x;
            movement_v.y = -movement_v.y;
        }
    }

    Player->rotation = 180 - round((atan2(GetMousePosition().x - center.x, GetMousePosition().y - center.y) / 3.1415)*180);
        
    // Update the positions
    Player->position.x += movement_v.x * (60 / (1/GetFrameTime()));
    if(check_collision(Player))
        Player->position.x -= movement_v.x * (60 / (1/GetFrameTime()));

    Player->position.y += movement_v.y * (60 / (1/GetFrameTime()));
    if(check_collision(Player))
        Player->position.y -= movement_v.y * (60 / (1/GetFrameTime()));
    
    // Mouse wheel
    mouse_wheel_v += GetMouseWheelMove()*tile_scale*4;
    if(mouse_wheel_v > 0 && tile_scale > 5 && !DEBUG)
        mouse_wheel_v = 0; 
    if(mouse_wheel_v < 0 && tile_scale < 1 && !DEBUG)
        mouse_wheel_v = 0;

    if(abs(mouse_wheel_v) <= 0.04) {
        if(mouse_wheel_v != 0)
            mouse_wheel_v = 0;
    }
    else {
        mouse_wheel_v = round(mouse_wheel_v * 8500)/10000;
        tile_scale += mouse_wheel_v/100;
    }
    
    // Update player selected tile
    float modx = float(
            pos_modulo(
                round(Player->position.x)
                , 50
            )
    ) / 50.0f;
    float mody = float(
            pos_modulo(
                round(Player->position.y)
                , 50
            )
    ) / 50.0f;

    int tilex = floor(round(Player->position.x) / 50);
    int tiley = floor(round(Player->position.y) / 50);

    int x = floor((GetMouseX() + modx*tile_w - center.x) / tile_w) + tilex;
    int y = floor((center.y - GetMouseY() + mody*tile_w) / tile_w + 1) + tiley;
    if(x != Player->select.x || y != Player->select.y) {
        Player->select = {x, y};
        Player->digging = false;
    }

    // Mouse buttons
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        if(!Player->digging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !tiles::is_air(World.get_tile((IntVec2){x, y}).id)) {
            Player->digging = true;
            Player->dig_progress = 0;
        }
        if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
            World.set_tile({ 
                (unsigned short)(Player->select.x%16), 
                (unsigned short)(Player->select.y%16) 
            }, { 
                (unsigned short)(Player->select.x/16), 
                (unsigned short)(Player->select.y/16) 
            },
            {tiles::ID::INSULATION, 1500});
    }

    if(Player->digging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        Player->digging = false;
}

int main() {
    Structure start_zone = LoadStructure("resources/structures/start_zone.struct");

    // Init window
    Vector2 base_size = {800, 500};
    Vector2 window_size = base_size;
    float center_shader_val[2] = {base_size.x/2, base_size.y/2};
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(window_size.x, window_size.y, "Game");
    
    // Init player
    _player Player = _player(&window_size);
    Player.position = { WORLD_SIZE * 25, WORLD_SIZE * 25 }; // Middle of the map

    // Init map
    World = world_map();
    World.generate_cave({(WORLD_SIZE/2), (WORLD_SIZE/2) - 3}, 10, 9, {tiles::ID::VACUMN, 0});
    World.generate();
    World.generate_cave((IntVec2){(WORLD_SIZE/2) + 3, (WORLD_SIZE/2) - 3}, 3, 3, {tiles::ID::SILT, 1200});
    World.place_structure(start_zone, {(WORLD_SIZE/2)-(start_zone.width/2), WORLD_SIZE/2-(start_zone.height/2)});

    // Load tile textures
    tiles::load(&Player);

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
            center_shader_val[0] = window_size.x/2;
            center_shader_val[1] = window_size.y/2;
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

            World.render(&Player, tile_w, tile_scale);

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
        handle_input(&Player, tile_w, {window_size.x/2, window_size.y/2});

        if(ticks != int(GetTime()*TPS)) {
            ticks = int(GetTime()*TPS);
            World.tick_update();
            Player.tick_update( tiles::density[World.get_tile(Player.select).id] );
        }

        // Update player digging status
        if(Player.dig_progress>=1) {
            World.set_tile({ 
                (unsigned short)(Player.select.x%16), 
                (unsigned short)(Player.select.y%16) 
            }, { 
                (unsigned short)(Player.select.x/16), 
                (unsigned short)(Player.select.y/16) 
            },
            {tiles::ID::VACUMN, 0});
            Player.dig_progress = 0;
            Player.digging = false;
        }
    }

    // Unload everything
    World.stop_update_thread();
    Player.unload();
    CloseWindow();
    return 0;
}
