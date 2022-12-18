#pragma once

// Raylib headers
#include <raylib.h>

#include "player.cpp"

// C++ headers
#include <iostream>
#include <math.h>

#include "vec2.h"
#include "../include/math+.h"

#define MAX_OVERLAYS 30

using namespace std;


#define TILE_COUNT 12

namespace tiles {
    enum {
        VOID,
        OXYGEN,
        VACUMN,
        STONE,
        SILT,
        COPPER,
        TITANIUM,
        INSULATION,
        REINFORCED_WINDOW,
        DOOR,
        DOOR_PANEL_A,
        DOOR_PANEL_B
    } typedef ID;

    float density[TILE_COUNT] = {
        1,
        1,
        1,
        0.8f,
        0.7f,
        0.9f,
        0.99f,
        0.99f,
        0.95f,
        0.99f,
        0.99f,
        0.99f
    };

    struct tile {
        unsigned short id;
        float mass = 1500;
    };

    tile VOID_TILE = {0, 0};

    Texture2D sprites[TILE_COUNT];
    Texture2D shade, select, oxygen;
    Texture2D dig_sprites[29];

    _player * player;

    RenderTexture2D shading_buffer;

    void load(_player * Player) {
        player = Player;

        string tile_path = "resources/images/tiles/";
        string overlay_path = "resources/images/overlays/";
        sprites[ID::OXYGEN] = LoadTexture( (tile_path + "ground.png").c_str() );
        sprites[ID::VACUMN] = LoadTexture( (tile_path + "ground.png").c_str() );
        sprites[ID::STONE] = LoadTexture( (tile_path + "stone.png").c_str() );
        sprites[ID::SILT] = LoadTexture( (tile_path + "silt.png").c_str() );
        sprites[ID::COPPER] = LoadTexture( (tile_path + "copper.png").c_str() );
        sprites[ID::TITANIUM] = LoadTexture( (tile_path + "titanium.png").c_str() );
        sprites[ID::INSULATION] = LoadTexture( (tile_path + "insulation.png").c_str() );
        sprites[ID::REINFORCED_WINDOW] = LoadTexture( (tile_path + "glass.png").c_str() );
        sprites[ID::DOOR] = LoadTexture( (tile_path + "door.png").c_str() );
        sprites[ID::DOOR_PANEL_A] = LoadTexture( (tile_path + "door_panel1.png").c_str() );
        sprites[ID::DOOR_PANEL_B] = LoadTexture( (tile_path + "door_panel2.png").c_str() );
        shade = LoadTexture( (overlay_path + "shade.png").c_str() );
        select = LoadTexture( (overlay_path + "select.png").c_str() );
        oxygen = LoadTexture( (overlay_path + "oxygen.png").c_str() );

        for(int i = 1;i<30;++i) {
            dig_sprites[i-1] = LoadTexture( (overlay_path + "break/" + to_string(i) + ".png").c_str() );
        }

        shading_buffer = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
    }

    void unload() {
        for(int i = 0; i < TILE_COUNT;++i)
            UnloadTexture(sprites[i]);
    }

    inline static bool is_air(unsigned short id) {
        return id == ID::OXYGEN || id == ID::VACUMN;
    }
    inline static bool is_transparent(unsigned short id) {
        return is_air(id) || id == ID::REINFORCED_WINDOW;
    }

    inline static bool has_wire_input(unsigned short id) {
        return id == ID::DOOR;
    }
    inline static bool has_wire_output(unsigned short id) {
        return id == ID::DOOR_PANEL_A || ID::DOOR_PANEL_B;
    }

    void draw_tile(tile tile, Vector2 pos, float scale, Color tint, short *wall, short next_to, bool selected) {
        if(tile.id == ID::VOID)
            return;

        if(selected)
            tint = WHITE;

        // Draw the ground behind transparent tiles
        if(is_transparent(tile.id))
            DrawTextureEx(sprites[ID::VACUMN], {
                GetRenderWidth()/2.0f + pos.x, 
                GetRenderHeight()/2.0f - pos.y
            }, 0, scale+0.01f // A bit larger to avoid any gaps
            , tint);

        // Draw the tile
        DrawTextureEx(sprites[tile.id], {
            GetRenderWidth()/2.0f + pos.x, 
            GetRenderHeight()/2.0f - pos.y
        }, 0, scale+0.01f // A bit larger to avoid any gaps
        , tint);

        // Check if the tile is selected
        if(selected) {
            // Draw the digging overlay
            if(!player->digging)
                DrawTextureEx(select, {
                    GetRenderWidth()/2.0f + pos.x, 
                    GetRenderHeight()/2.0f - pos.y
                }, 0, scale*2+0.01f // A bit larger to avoid any gaps
                , tint);
            // Draw the selection overlay
            else
                DrawTextureEx(dig_sprites[ (int)(player->dig_progress*29) ], {
                    GetRenderWidth()/2.0f + pos.x, 
                    GetRenderHeight()/2.0f - pos.y
                }, 0, scale*2+0.01f // A bit larger to avoid any gaps
                , tint);
        }

        // Shading and tint
        if(is_air(tile.id)) {
            // Overlay the gas texture
            unsigned char shade_alpha = 150;
            BeginTextureMode(shading_buffer);
            if(tile.id == ID::OXYGEN) {
                DrawTexturePro(
                    oxygen,
                    {0, 0, (float)shade.width, (float)shade.height},
                    {
                        GetRenderWidth()/2.0f + pos.x+ 8*scale, 
                        GetRenderHeight()/2.0f + pos.y - 8*scale,
                        16*scale,
                        16*scale
                    },
                    { 8*scale, 8*scale },
                    0,
                    (Color){255, 255, 255, (unsigned char)clamp(tile.mass / 9.5f, 0, 200)}
                );
                shade_alpha = 150 - clamp(tile.mass / 20.5f, 0, 150);
            }
            
            // Wall shading
            if (next_to) {
                for(int i = 0;i<next_to;++i)
                    DrawTexturePro(
                    shade,
                    {0, 0, (float)shade.width, (float)shade.height},
                    {
                        GetRenderWidth()/2.0f + pos.x+ 8*scale, 
                        GetRenderHeight()/2.0f + pos.y - 8*scale,
                        16*scale,
                        16*scale
                    },
                    { 8*scale, 8*scale },
                    wall[i],
                    (Color){255, 255, 255, shade_alpha}
                    );
            }
            EndTextureMode();
        }
    }
}