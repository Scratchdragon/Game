#pragma once

// Raylib headers
#include <raylib.h>

#include "player.cpp"

// C++ headers
#include <iostream>
#include <math.h>

#include "vec2.h"

#define MAX_OVERLAYS 30

using namespace std;


#define TILE_COUNT 8

namespace tiles {
    enum {
        VOID,
        GROUND,
        CHARRED_GROUND,
        STONE,
        SILT,
        COPPER,
        TITANIUM,
        INSULATION
    } typedef ID;

    Texture2D sprites[TILE_COUNT];
    Texture2D shade;

    RenderTexture2D shading_buffer;

    void load() {
        string image_path = "resources/images/tiles/";
        sprites[ID::GROUND] = LoadTexture( (image_path + "ground.png").c_str() );
        sprites[ID::CHARRED_GROUND] = LoadTexture( (image_path + "charred_ground.png").c_str() );
        sprites[ID::STONE] = LoadTexture( (image_path + "stone.png").c_str() );
        sprites[ID::SILT] = LoadTexture( (image_path + "silt.png").c_str() );
        sprites[ID::COPPER] = LoadTexture( (image_path + "copper.png").c_str() );
        sprites[ID::TITANIUM] = LoadTexture( (image_path + "titanium.png").c_str() );
        sprites[ID::INSULATION] = LoadTexture( (image_path + "insulation.png").c_str() );
        shade = LoadTexture( (image_path + "shade.png").c_str() );
        //shade = LoadTextureFromImage(GenImageGradientH(16, 16, (Color){0,0,0,0}, (Color){0, 0, 0, 200}));

        shading_buffer = LoadRenderTexture(GetRenderWidth(), GetRenderHeight());
    }

    void unload() {
        for(int i = 0; i < TILE_COUNT;++i)
            UnloadTexture(sprites[i]);
    }

    bool is_air(unsigned short id) {
        return id == ID::GROUND || id == ID::CHARRED_GROUND;
    }

    void draw_tile(int tile, Vector2 pos, float scale, Color tint, short *wall, short next_to) {
        if(tile == ID::VOID)
            return;
        
        DrawTextureEx(sprites[tile], {
            GetRenderWidth()/2.0f + pos.x, 
            GetRenderHeight()/2.0f - pos.y
        }, 0, scale+0.01f // A bit larger to avoid any gaps
        , tint);

        if(is_air(tile) && next_to) {
            BeginTextureMode(shading_buffer);
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
                    (Color){255, 255, 255, 150}
                );
            EndTextureMode();
        }
    }
}