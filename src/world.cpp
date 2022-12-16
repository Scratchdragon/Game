#include <raylib.h>

#include <map>
#include <iostream>

// For big vector2
#include "vec2.h"

// For better map size output
#include "byte_util.h"

// For tile operations
#include "tiles.cpp"
#include "player.cpp"
#include "random.h"

using namespace std;

class world_map {
    public:

    unsigned short generate_tile(long long key) {
        float f_rand = Random::Rand(key);

        float max = 0;
        for(int i = 0; i < TILE_COUNT; ++i) {
            max += tiles::weights[i];
            if(f_rand < max) return i;
        }
        return TILE_COUNT-1;
    }

    inline long long unique_id(LongVec2 pos) {
        // Id is only unique per quadrant so this adds some randomness per quadrant
        long long id = pos.id();
        if(pos.x < 0)
            id += Random::Long(0, 100, 10000);
        if(pos.y < 0)
            id = -id;
        return id;
    }

    unsigned short get_tile(LongVec2 pos) {
        return generate_tile(
            unique_id(pos)
        );
    }

    // How many extra tiles to render
    //                      left  right  top  bottom
    Vector4 r_padding = {  2,     2,    2,    9};

    void render(_player * player, float size, float scale, Color tint) {
        tiles::overlays = 0;

        // Get the tile position of the player
        int tilex = floor(round(player->position.x) / 44);
        int tiley = floor(round(player->position.y) / 44);
        float modx = float(
            pos_modulo(
                round(player->position.x)
                , 44
            )
        ) / 44.0f;
        float mody = float(
            pos_modulo(
                round(player->position.y)
                , 44
            )
        ) / 44.0f;

        // The screen size in tiles
        int tilew = ceil(GetRenderWidth()/size);
        int tileh = ceil(GetRenderHeight()/size);

        for(int y = (-tileh/2) - r_padding.z; y < (tileh/2) + r_padding.w; ++y) {
            for(int x = (-tilew/2) - r_padding.x; x < (tilew/2) + r_padding.y; ++x) {
                tiles::draw_tile( 
                    get_tile(
                        { (long)(tilex + x), (long)(tiley + y) }
                    ),
                    {(x * size) - (modx * size), (y * size) + size - (mody * size)}, // Offset because sprites are drawn from the bottom
                    scale,
                    tint
                );
            }
        }

        tiles::render_overlays(scale, player, size, tint);
    }
};