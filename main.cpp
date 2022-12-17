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

bool debug = true;

float tile_scale = 2.0f;
float mouse_wheel_v = 0;

Vector2 movement_v = {0, 0};

world_map World;

bool check_collision(_player * Player) {
    Player->collision = 
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 1.4)})) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 0.6)})) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 1.4)})) ||
    !tiles::is_air(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 0.6)}));
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
    
    // Setup the shader
    Shader shader = LoadShader(TextFormat("src/shaders/glsl%i-base.vs", GLSL_VERSION), TextFormat("src/shaders/glsl%i-lighting.fs", GLSL_VERSION));
    int shader_rotation_loc = GetShaderLocation(shader, "rotation");
    int shader_center_loc = GetShaderLocation(shader, "center");
    int shader_scale_loc = GetShaderLocation(shader, "scale");
    SetShaderValue(shader, shader_rotation_loc, &Player.rotation, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, shader_rotation_loc, center_shader_val, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, shader_scale_loc, &tile_scale, SHADER_UNIFORM_FLOAT);

    // Init map
    World = world_map();
    World.generate_cave({(WORLD_SIZE/2), (WORLD_SIZE/2) - 3}, 18, 50, tiles::ID::GROUND);
    World.generate();
    World.generate_cave((IntVec2){(WORLD_SIZE/2) + 3, (WORLD_SIZE/2) - 3}, 3, 3, tiles::ID::SILT);
    World.place_structure(start_zone, {(WORLD_SIZE/2)-(start_zone.width/2), WORLD_SIZE/2-(start_zone.height/2)});

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
    
            BeginShaderMode(shader);
            World.render(&Player, tile_w, tile_scale);
            EndShaderMode();

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

        // Update the shader
        SetShaderValue(shader, shader_rotation_loc, &Player.rotation, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, shader_center_loc, center_shader_val, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, shader_scale_loc, &tile_scale, SHADER_UNIFORM_FLOAT);
    }

    // Unload everything
    Player.unload();
    CloseWindow();
    return 0;
}
