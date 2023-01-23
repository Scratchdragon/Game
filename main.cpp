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

#define XBOX360_LEGACY_NAME_ID  "Xbox Controller"
#if defined(PLATFORM_RPI)
    #define XBOX360_NAME_ID     "Microsoft X-Box 360 pad"
    #define PS3_NAME_ID         "PLAYSTATION(R)3 Controller"
#else
    #define XBOX360_NAME_ID     "Xbox 360 Controller"
    #define PS3_NAME_ID         "PLAYSTATION(R)3 Controller"
#endif

#define DEBUG false
#define TPS 10

float tile_scale = 2.0f;

// Global variables for input
Vector2 movement_v = {0, 0};
float mouse_wheel_v = 0;
bool gamepad_available = false;
Vector2 last_mouse;
bool using_gamepad = false;
int gamepad_released = 0;
int gamepad_pressed = 0;
bool rtrigger;
bool ltrigger;
float last_axis;
Vector2 mouse;

// Global world variables
world_map World;
long long ticks; // The ticks elapsed since game start

bool check_collision(_player * Player) {
    Player->collision = 
    tiles::is_collidable(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 1.4)}).id) ||
    tiles::is_collidable(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 0.6)}).id) ||
    tiles::is_collidable(World.get_tile({(int)(Player->position.x/50 - 0.4), (int)(Player->position.y/50 + 1.4)}).id) ||
    tiles::is_collidable(World.get_tile({(int)(Player->position.x/50 + 0.4), (int)(Player->position.y/50 + 0.6)}).id);
    return Player->collision;
}

void handle_input(_player * Player, float tile_w, Vector2 center) {
    // Check controller or keyboard
    if (using_gamepad && (last_mouse.x != GetMousePosition().x || last_mouse.y != GetMousePosition().y || GetKeyPressed())) {
        using_gamepad = false;
        cout << "GAME: Input set keyboard" << endl;
        EnableCursor();
    }
    else if(!using_gamepad && gamepad_available) {
        float axis_total = 0;
        for (int i = 0; i < GetGamepadAxisCount(0); i++) {
            axis_total += GetGamepadAxisMovement(0, i);
        }
        if(axis_total != last_axis || GetGamepadButtonPressed() != -1) {
            using_gamepad = true;
            cout << "GAME: Input set to gamepad 0" << endl;
            DisableCursor();
        }
    }
    last_mouse = GetMousePosition();
    if(gamepad_available) {
        last_axis = 0;
        for (int i = 0; i < GetGamepadAxisCount(0); i++) {
            last_axis += GetGamepadAxisMovement(0, i);
        }
        gamepad_released = 0;
        if(gamepad_pressed != GetGamepadButtonPressed()) {
            gamepad_released = gamepad_pressed;
            gamepad_pressed = GetGamepadButtonPressed();
        }
    }

    // Slow down
    movement_v.x /= 1.15;
    movement_v.y /= 1.15;

    // Basic movement keys
    if ((IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) && !using_gamepad) {
        movement_v.x = sin(Player->rotation/(180/PI)) * PLAYER_SPEED;
        movement_v.y = cos(Player->rotation/(180/PI)) * PLAYER_SPEED;
        if(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
            movement_v.x = -movement_v.x;
            movement_v.y = -movement_v.y;
        }
    }

    if(!using_gamepad)
        Player->rotation = 180 - round((atan2(GetMousePosition().x - center.x, GetMousePosition().y - center.y) / 3.1415)*180);
    else if(GetGamepadAxisMovement(0, 0) + GetGamepadAxisMovement(0, 1)) {
        Player->rotation = 180 - round((atan2(GetGamepadAxisMovement(0, 0), GetGamepadAxisMovement(0, 1)) / 3.1415)*180);
        movement_v.x = sin(Player->rotation/(180/PI)) * PLAYER_SPEED;
        movement_v.y = cos(Player->rotation/(180/PI)) * PLAYER_SPEED;
    }
        
    // Update the positions
    Player->position.x += movement_v.x * (60 / (1/GetFrameTime()));
    if(check_collision(Player))
        Player->position.x -= movement_v.x * (60 / (1/GetFrameTime()));

    Player->position.y += movement_v.y * (60 / (1/GetFrameTime()));
    if(check_collision(Player))
        Player->position.y -= movement_v.y * (60 / (1/GetFrameTime()));

    // Update mouse location
    if(!using_gamepad)
        mouse = (Vector2){GetMouseX(), GetMouseY()};
    else {
        mouse.x += GetGamepadAxisMovement(0, 2)*2.5*tile_scale;
        mouse.y += GetGamepadAxisMovement(0, 3)*2.5*tile_scale;
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

    int x = floor((mouse.x + modx*tile_w - center.x) / tile_w) + tilex;
    int y = floor((center.y - mouse.y + mody*tile_w) / tile_w + 1) + tiley;
    if(x != Player->select.x || y != Player->select.y) {
        Player->select = {x, y};
        Player->digging = false;
        Player->interact = false;
        Player->dig_progress = 0;
    }

    // Mouse wheel
    mouse_wheel_v += GetMouseWheelMove()*tile_scale*4;

    // Mouse buttons
    if(!using_gamepad) {
        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            if(!Player->digging && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !tiles::is_air(World.get_tile((IntVec2){x, y}).id)) {
                Player->digging = true;
                Player->dig_progress = 0;
            }
            if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && tiles::is_air(World.get_tile((IntVec2){x, y}).id))
                World.set_tile({ 
                    (unsigned short)(Player->select.x%16), 
                    (unsigned short)(Player->select.y%16) 
                }, { 
                    (unsigned short)(Player->select.x/16), 
                    (unsigned short)(Player->select.y/16) 
                },
                {tiles::ID::INSULATION, 1500});
        }
        if(IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && !Player->interact)
            Player->interact = true;

        if(Player->digging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            Player->digging = false;
    }
    else {
        if(GetGamepadAxisCount(0) > 4) {
            if(!ltrigger && GetGamepadAxisMovement(0, 4) > 0)
                Player->interact = true;
            Player->digging = rtrigger;
            rtrigger = GetGamepadAxisMovement(0, 5) > 0;
            ltrigger = GetGamepadAxisMovement(0, 4) > 0;
        }
        switch(GetGamepadButtonPressed()) {
            case 9:
                mouse_wheel_v = 3;
                break;
            case 11:
            mouse_wheel_v = -3;
                break;
        }
    }

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

    // Controller input
    if(IsGamepadAvailable(0)) {
        if (!gamepad_available) {
            gamepad_available = true;
            cout << "GAME: Detected controller " << GetGamepadName(0) << endl;

        }
    }
    else if(gamepad_available) {
        gamepad_available = false;
        cout << "GAME: Lost connection to controller" << endl;
        using_gamepad = false;
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

    // Init map
    World = world_map();
    World.mouse = &mouse;
    World.generate();
    World.generate_cave({(WORLD_SIZE/2), (WORLD_SIZE/2) - 3}, 10, 9, {tiles::ID::OXYGEN, 1400});
    World.generate_cave((IntVec2){(WORLD_SIZE/2), (WORLD_SIZE/2) - 6}, 3, 3, {tiles::ID::SILT, 1200});
    
    World.place_structure(start_zone, {(WORLD_SIZE/2)-(start_zone.width/2), WORLD_SIZE/2-(start_zone.height/2)});
    

    // Load textures
    tiles::load(&Player);
    Texture2D cursor = LoadTexture("resources/images/ui/cursor.png");

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
            DrawTexture(cursor, mouse.x, mouse.y, WHITE);

            DrawText( to_string((int)fps).c_str(), 4, 4, 20, RAYWHITE);

        EndDrawing();
        
        // Update time data
        render_end = clock();
        fps = ((fps*30) + (1/GetFrameTime())) / 31;
        avg_wr_ms = ((avg_wr_ms*10) + (float(wr_end-wr_start) / float(CLOCKS_PER_SEC))) / 11;
        avg_ms = ((avg_ms*10) + (float(render_end-render_start) / float(CLOCKS_PER_SEC))) / 11;

        // Update player
        Player.size = {16 * tile_scale, 16 * tile_scale};

        // Handle the user input
        handle_input(&Player, tile_w, {window_size.x/2, window_size.y/2});

        if(ticks != int(GetTime()*TPS)) {
            ticks = int(GetTime()*TPS);
            Player.tick_update( tiles::tile_prefabs[World.get_tile(Player.select).id].density);
            IntVec2 player_pos = (IntVec2){(int)(Player.position.x/50), (int)(Player.position.y/50)+1};
            tiles::tile t = World.get_tile(player_pos);
            if(t.id == tiles::ID::OXYGEN)
                World.set_mass(player_pos, t.mass - 10);
            if(t.mass < 0)
                World.set_mass(player_pos, 0);
            World.tick_update(&Player);
            
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

            // Decrease load if framerate is struggling
            if(fps <= 30) max_light_dist = fps/2;
            else max_light_dist = 15;
        }
    }

    // Unload everything
    World.stop_update_thread();
    Player.unload();
    CloseWindow();
    return 0;
}
