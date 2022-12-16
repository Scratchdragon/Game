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

#define CAVE_COUNT 50
#define MAX_CAVE_LEN 12
#define MIN_CAVE_LEN  4
#define MAX_CAVE_SIZE 18
#define MIN_CAVE_SIZE 3

#define WORLD_SIZE 400 // Max of 4000

using namespace std;

struct chunk {
    long id;
    unsigned short content[16][16];
    const unsigned short * operator []( const short x ) const {
        return content[x];
    }
};

class world_map {
    public:

    map<UShortVec2, chunk> chunkmap;

    void fill_circle(IntVec2 pos, int radius, unsigned short tile) {
        for(int x = -radius/2;x<radius/2;++x) {
            for(int y = -radius/2;y<radius/2;++y) {
                if(pos.x + x < 0 || pos.y + y < 0 || pos.x + x > WORLD_SIZE || pos.y + y > WORLD_SIZE)
                    continue;
                if(dist(pos, (IntVec2){pos.x + x, pos.y + y}) < radius/2) {
                    set_tile({ 
                        (unsigned short)((pos.x+x)%16), 
                        (unsigned short)((pos.y+y)%16) 
                    }, { 
                        (unsigned short)((pos.x+x)/16), 
                        (unsigned short)((pos.y+y)/16) 
                    }, tile);
                }
            }
        }
    }

    void generate_cave(IntVec2 pos, float size, int len) {
        float direction = (float(rand())/float(RAND_MAX))*(PI*2);
        float sv = 0;
        Vector2 loc = {(float)pos.x, (float)pos.y};
        for(int i = 0; i < len; ++i) {
            loc.x+=sin(direction)*(size/3);
            loc.y+=cos(direction)*(size/3);
            direction += (float(rand())/float(RAND_MAX) - 0.5f)*(PI/2);
            sv+=(float(rand())/float(RAND_MAX) - 0.5);
            size+=sv;
            if(size > MAX_CAVE_SIZE || size < MIN_CAVE_SIZE) {
                size-=sv;
                sv=0;
            }
            fill_circle({(int)loc.x, (int)loc.y}, size, tiles::ID::GROUND);
        }
    }

    void generate() {
        for(int i = 0;i<CAVE_COUNT;++i) {
            IntVec2 pos = {rand()%WORLD_SIZE, rand()%WORLD_SIZE};
            while(dist(pos, (IntVec2){WORLD_SIZE/2, WORLD_SIZE/2}) < 60)
                pos = {rand()%WORLD_SIZE, rand()%WORLD_SIZE};
            generate_cave(pos, (rand()%(MAX_CAVE_SIZE-MIN_CAVE_SIZE))+MIN_CAVE_SIZE, (rand()%(MAX_CAVE_LEN-MIN_CAVE_LEN))+MIN_CAVE_LEN);
            cout << "Generating World: " << round((float(i)/float(CAVE_COUNT))*1000)/10 << "%\n";
        }
    }

    void set_tile(UShortVec2 rel_pos, UShortVec2 c_pos, unsigned short id) {
        auto c = chunkmap.find(c_pos);
        if(c == chunkmap.end()) {
            chunkmap.insert(pair<UShortVec2, chunk>( c_pos, chunk() ));
            c = chunkmap.find(c_pos); // Get the chunk again
        }
        c->second.content[rel_pos.x][rel_pos.y] = id;
    }

    unsigned short create_tile(UShortVec2 rel_pos, UShortVec2 c_pos) {
        unsigned short id = 0;
        float r = float(rand())/float(RAND_MAX);

        if(r < 0.995)
            id = tiles::ID::STONE;
        else
            id = tiles::ID::TITANIUM;

        set_tile(rel_pos, c_pos, id);
        return id;
    }

    unsigned short get_tile(IntVec2 pos) {
        if(pos.x<0 || pos.y<0 || pos.x>WORLD_SIZE || pos.y>WORLD_SIZE)
            return 0;

        auto c = chunkmap.find( (UShortVec2){ 
            (unsigned short)(pos.x/16), 
            (unsigned short)(pos.y/16) 
        } );

        // Create a new tile if the selected tile or its chunk does not exist
        if(c == chunkmap.end() || c->second[pos.x%16][pos.y%16] == 0) {
            return create_tile({ 
                (unsigned short)(pos.x%16), 
                (unsigned short)(pos.y%16) 
            }, { 
                (unsigned short)(pos.x/16), 
                (unsigned short)(pos.y/16) 
            });
        }

        return c->second[pos.x%16][pos.y%16];
    }

    // How many extra tiles to render
    //                      left  right  top  bottom
    Vector4 r_padding = {  2,     2,    2,    9};

    void render(_player * player, float size, float scale, Color tint) {
        // Get the tile position of the player
        int tilex = floor(round(player->position.x) / 50);
        int tiley = floor(round(player->position.y) / 50);
        float modx = float(
            pos_modulo(
                round(player->position.x)
                , 50
            )
        ) / 50.0f;
        float mody = float(
            pos_modulo(
                round(player->position.y)
                , 50
            )
        ) / 50.0f;

        // The screen size in tiles
        int tilew = ceil(GetRenderWidth()/size);
        int tileh = ceil(GetRenderHeight()/size);

        unsigned short tile = 0;
        short *wall;
        short i = 0;
        for(int y = (-tileh/2) - r_padding.z; y < (tileh/2) + r_padding.w; ++y) {
            for(int x = (-tilew/2) - r_padding.x; x < (tilew/2) + r_padding.y; ++x) {
                tile = get_tile({ (tilex + x), (tiley + y) });
                i=0;
                if(tiles::is_air(tile)) {
                    wall = new short[4];
                    if(!tiles::is_air(get_tile({tilex + x + 1, tiley + y}))) {
                        wall[i] = 0;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x - 1, tiley + y}))) {
                        wall[i] = 180;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x, tiley + y + 1}))) {
                        wall[i] = 90;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x, tiley + y - 1}))) {
                        wall[i] = 270;
                        ++i;
                    }
                }
                    
                tiles::draw_tile( 
                    tile,
                    {(x * size) - (modx * size), (y * size) - (mody * size)}, // Offset because sprites are drawn from the bottom
                    scale,
                    tint,
                    wall,
                    i
                );
            }
        }
    }
};