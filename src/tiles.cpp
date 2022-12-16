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


#define TILE_COUNT 6

namespace tiles {
    enum {
        AIR,
        GRASS1,
        GRASS2,
        GRASS3,
        MUSHROOMS,
        TREE
    } typedef ID;

    float weights[TILE_COUNT] = { // The chances of each tile
        0,
        0.58f,
        0.2f,
        0.2f,
        0.01f,
        0.01f
    };

    
    Texture2D sprites[TILE_COUNT];

    // Additional sprites
    Texture2D tree;

    void load() {
        string image_path = "resources/images/tiles/";

        // Load grass textures
        for(int i = 1; i < 4; ++i) {
            string path = image_path + "grass" + to_string(i) + ".png";
            sprites[i] = LoadTexture(path.c_str());
        }

        sprites[ID::MUSHROOMS] = LoadTexture( (image_path + "shrooms.png").c_str() );
        sprites[ID::TREE] = LoadTexture( (image_path + "tree_grass.png").c_str() );
        tree = LoadTexture( (image_path + "tree.png").c_str() );
    }

    void unload() {
        for(int i = 0; i < TILE_COUNT;++i) {
            UnloadTexture(sprites[i]);
        }
    }

    unsigned short overlays = 0;
    Texture2D * render_buffer[MAX_OVERLAYS];
    Vector2 render_buffer_pos[MAX_OVERLAYS];

    void draw_tile(int tile, Vector2 pos, float scale, Color tint) {
        if(tile == ID::AIR)
            return;
        
        DrawTextureEx(sprites[tile], {
            GetRenderWidth()/2.0f + pos.x, 
            GetRenderHeight()/2.0f + pos.y - (sprites[tile].height * scale) // Render from the bottom of the image to allow overlapping
        }, 0, scale+0.01f // A bit larger to avoid any gaps
        , tint);

        if(tile == ID::TREE) {
            if (overlays == MAX_OVERLAYS-1)
                cout << "Overlay limit reached (" << overlays+1 << "/" << MAX_OVERLAYS << ")\n";
            else { 
                if(overlays > MAX_OVERLAYS-5)
                    cout << "Almost reached overlay limit (" << overlays+1 << "/" << MAX_OVERLAYS << ")\n";
                render_buffer[overlays] = &tree;
                render_buffer_pos[overlays] = pos;
                ++overlays;
            }
        }
    }

    void render_overlays(float scale, _player * player, float tile_w, Color tint) {

        // This just checks if the first overlay is in front of the player and renders them if they are
        bool drew_player = (
            render_buffer_pos[0].y >= player->size.y/2 + tile_w/1.25f
        ); 
        if (drew_player)
            player->render();
        
        for(int i = 0;i<overlays;++i) {
            Vector2 size = {
                (float)render_buffer[i]->width * scale,
                (float)render_buffer[i]->height * scale
            };

            DrawTexturePro(
                *render_buffer[i],
                { // The initial image dimensions (x, y, width, height)
                    0, 
                    0, 
                    (float)render_buffer[i]->width, 
                    (float)render_buffer[i]->height 
                },
                { // The new image dimensions 
                    GetRenderWidth()/2.0f + render_buffer_pos[i].x + tile_w/1.75f, 
                    GetRenderHeight()/2.0f + render_buffer_pos[i].y - tile_w/1.25f,
                    size.x,
                    size.y
                },
                {size.x/2, size.y}, // Where to draw the image from
                sin((GetTime()/2) + (round(render_buffer_pos[i].y + player->position.y) / 400)) * 2, // Tree swaying
                tint
            );

            // Draw the player if the next tree is in front of the player
            if(!drew_player && i<overlays-1 && render_buffer_pos[i+1].y >= player->size.y + tile_w/3.8f) {
                drew_player = true;
                player->render();
            }
        }

        // Draw the player if they havent already been
        if(!drew_player) player->render();
    }
}