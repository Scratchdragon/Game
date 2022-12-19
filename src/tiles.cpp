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


#define TILE_COUNT 14

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
        DOOR_PANEL_B,
        DOOR_OPEN,
        GAS_OUTLET
    } typedef ID;

    string tile_path = "resources/images/tiles/";
    string overlay_path = "resources/images/overlays/";

    struct tile_prefab {
        // Initial values
        string name;
        
        // Sprite
        string sprite;

        // Initial values
        float mass;
        float density;
    
        // Flags
        bool transparent = false;
        bool gas = false;
        bool air_tight = true;
        bool can_collide = true;
    };
    const static tile_prefab tile_prefabs[TILE_COUNT] = {
        {"Void", "", 0, 0, false, false},
        {"Oxygen", tile_path + "ground.png", 1500, 1, true, true},
        {"Vacumn", tile_path + "ground.png", 0, 1, true, true},
        {"Stone", tile_path + "stone.png", 1400, 0.8f},
        {"Silt", tile_path + "silt.png", 1600, 0.7f},
        {"Copper", tile_path + "copper.png", 1200, 0.9f},
        {"Titanium", tile_path + "titanium.png", 1000, 0.99f},
        {"Insulated Wall", tile_path + "insulation.png", 1500, 0.99f},
        {"Reinforced Window", tile_path + "glass.png", 1200, 0.95f, true},
        {"Door", tile_path + "door.png", 1600,  0.99f},
        {"Door Panel", tile_path + "door_panel1.png", 1600,  0.99f},
        {"Door Panel", tile_path + "door_panel2.png", 1600,  0.99f},
        {"Door", tile_path + "door_open.png", 1600,  0.99f, true, false, false, false},
        {"Gas Outlet", tile_path + "gas_outlet.png", 1000,  0.9f, true, false, false}
    };
    
    struct tile {
        unsigned short id;
        float mass = 1500;
    };

    struct data_tile {
    };

    tile VOID_TILE = {0, 0};

    Texture2D sprites[TILE_COUNT];
    Texture2D shade, select, oxygen, gas_shade;
    Texture2D dig_sprites[29];

    _player * player;

    RenderTexture2D shading_buffer;

    void load(_player * Player) {
        player = Player;

        for(int i = 1;i<TILE_COUNT;++i) {
            sprites[i] = LoadTexture(tile_prefabs[i].sprite.c_str());
        }

        shade = LoadTexture( (overlay_path + "shade.png").c_str() );
        select = LoadTexture( (overlay_path + "select.png").c_str() );
        oxygen = LoadTexture( (overlay_path + "oxygen.png").c_str() );
        gas_shade = LoadTexture( (overlay_path + "oxygen-shade.png").c_str() );

        for(int i = 1;i<30;++i) {
            dig_sprites[i-1] = LoadTexture( (overlay_path + "break/" + to_string(i) + ".png").c_str() );
        }

        shading_buffer = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
    }

    void unload() {
        for(int i = 0; i < TILE_COUNT;++i)
            UnloadTexture(sprites[i]);
    }

    tile from_id(unsigned short id) {
        return (tile){id, tile_prefabs[id].mass};
    }

    inline static bool is_air(unsigned short id) {
        return tile_prefabs[id].gas;
    }
    inline static bool is_transparent(unsigned short id) {
        return tile_prefabs[id].transparent;
    }
    inline static bool is_not_airtight(unsigned short id) {
        return !tile_prefabs[id].air_tight;
    }
    inline static bool is_collidable(unsigned short id) {
        return tile_prefabs[id].can_collide && !is_air(id);
    }

    inline static bool has_wire_input(unsigned short id) {
        return id == ID::DOOR;
    }
    inline static bool has_wire_output(unsigned short id) {
        return id == ID::DOOR_PANEL_A || ID::DOOR_PANEL_B;
    }

    void draw_tile(tile tile, Vector2 pos, float scale, Color tint, int *wall, short next_to, bool selected, tiles::tile gas = tiles::VOID_TILE) {
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

        // Non airtight overlay
        if(is_not_airtight(tile.id) && gas.id != 0) {
            // Overlay the gas texture
            Texture2D gas_texture;
            if(gas.id == ID::OXYGEN)
                gas_texture = oxygen;
            BeginTextureMode(shading_buffer);
            DrawTexturePro(
                gas_texture,
                {0, 0, (float)shade.width, (float)shade.height},
                {
                    GetRenderWidth()/2.0f + pos.x+ 8*scale, 
                    GetRenderHeight()/2.0f + pos.y - 8*scale,
                    16*scale,
                    16*scale
                },
                { 8*scale+0.01f, 8*scale+0.01f },
                0,
                (Color){255, 255, 255, (unsigned char)clamp(gas.mass / 9.5f, 0, 200)}
            );
            EndTextureMode();
        }

        // Shading and tint
        if(is_air(tile.id)) {
            // Overlay the gas texture
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
                    { 8*scale+0.01f, 8*scale+0.01f },
                    0,
                    (Color){255, 255, 255, (unsigned char)clamp(tile.mass / 9.5f, 0, 200)}
                );
            }
            
            // Wall shading
            if (next_to) {
                for(int i = 0;i<next_to;++i)
                    DrawTexturePro(
                    tile.id == ID::OXYGEN && tile.mass > 1100 ? gas_shade : shade,
                    {0, 0, (float)shade.width, (float)shade.height},
                    {
                        GetRenderWidth()/2.0f + pos.x+ 8*scale, 
                        GetRenderHeight()/2.0f + pos.y - 8*scale,
                        16*scale+0.01f,
                        16*scale+0.01f
                    },
                    { 8*scale, 8*scale },
                    wall[i],
                    (Color){255, 255, 255, (unsigned char)(150)}
                    );
            }
            EndTextureMode();
        }
    }
}