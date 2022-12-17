#include <raylib.h>

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

// For big vector2
#include "vec2.h"

// For better map size output
#include "byte_util.h"

// For tile operations
#include "tiles.cpp"
#include "player.cpp"
#include "random.h"

#define CAVE_COUNT 100
#define MAX_CAVE_LEN 12
#define MIN_CAVE_LEN  4
#define MAX_CAVE_SIZE 10
#define MIN_CAVE_SIZE 3

#define ORES 1000
#define DEPOSITS 500

#define DARKNESS 20
#define LIMIT_LIGHTING false

#define WORLD_SIZE 500 // Max of 4000

using namespace std;

struct chunk {
    long id;
    unsigned short content[16][16];
    const unsigned short * operator []( const short x ) const {
        return content[x];
    }
};

struct tile_column {
    unsigned short *column;
};

struct Structure {
    string src;
    int width, height;
    tile_column * content; // This data is definitly not readable, the string is just to make usage easier

    unsigned short &operator []( const UShortVec2 pos ) const {
        return content[pos.x].column[pos.y];
    }
};

Structure LoadStructure(string filename) {
    Structure s;
    s.src = filename;

    ifstream file(filename);

    if (file.is_open()) {
        string line;

        s.height = 1 + count(istreambuf_iterator<char>(file), istreambuf_iterator<char>(), '\n');

        string * content = new string[s.height];

        file.clear();
        file.seekg(0);

        int i = 0;
        while (getline(file, line)) {
            // Remove the spaces
            line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
            content[i] = line;
            ++i;
        }
        file.close();

        s.width = content[0].length();

        // Convert into unsigned short **
        s.content = new tile_column[s.width];

        for(unsigned short x = 0;x<s.width;++x) {
            s.content[x].column = new unsigned short[s.height];
            for(unsigned short y = 0;y<s.height;++y) {
                s[(UShortVec2){x, y}] = content[s.height-y-1][x] - 48;
            }
        }

        cout << "Loaded structure (" << s.width << "x" << s.height << ")\n";
    }
    return s;
}

class world_map {
    public:

    bool log = false;

    map<UShortVec2, chunk> chunkmap;

    void place_structure(Structure s, IntVec2 pos) {
        for(int x = pos.x;x<pos.x+s.width;++x) {
            for(int y = pos.y;y<pos.y+s.height;++y) {
                set_tile(
                    (UShortVec2){(unsigned short)(x%16), (unsigned short)(y%16)},
                    (UShortVec2){(unsigned short)(x/16), (unsigned short)(y/16)},
                    s[(UShortVec2){(unsigned short)(x-pos.x), (unsigned short)(y-pos.y)}]
                );
            }
        }
    }

    void fill_circle(IntVec2 pos, int radius, unsigned short tile, int randomize) {
        for(int x = -radius/2;x<radius/2;++x) {
            for(int y = -radius/2;y<radius/2;++y) {
                if(pos.x + x < 0 || pos.y + y < 0 || pos.x + x > WORLD_SIZE || pos.y + y > WORLD_SIZE)
                    continue;

                if(dist(pos, (IntVec2){pos.x + x, pos.y + y}) <= radius/2 && !(randomize && (rand()%randomize))) {
                    if(tiles::is_air(tile) || get_tile(pos) == tiles::ID::STONE)
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

    void generate_cave(IntVec2 pos, float size, int len, unsigned short tile) {
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
                
                sv = -sv;
            }
            fill_circle({(int)loc.x, (int)loc.y}, size, tile, 0);
        }
    }

    void generate() {
        for(int i = 0;i<CAVE_COUNT;++i) {
            IntVec2 pos = {rand()%WORLD_SIZE, rand()%WORLD_SIZE};
            while(dist(pos, (IntVec2){WORLD_SIZE/2, WORLD_SIZE/2}) < 60)
                pos = {rand()%WORLD_SIZE, rand()%WORLD_SIZE};
            generate_cave(pos, (rand()%(MAX_CAVE_SIZE-MIN_CAVE_SIZE))+MIN_CAVE_SIZE, (rand()%(MAX_CAVE_LEN-MIN_CAVE_LEN))+MIN_CAVE_LEN, tiles::ID::GROUND);
            cout << "Generating World: " << round((float(i)/float(CAVE_COUNT))*1000)/10 << "%\n";
        }

        for(int i = 0;i<ORES;++i) {
            fill_circle((IntVec2){rand()%WORLD_SIZE, rand()%WORLD_SIZE}, 1 + (rand()%4), tiles::ID::COPPER, 2);
        }

        for(int i = 0;i<DEPOSITS;++i) {
            generate_cave((IntVec2){rand()%WORLD_SIZE, rand()%WORLD_SIZE}, 2+(rand()%4), 2+(rand()%2), tiles::ID::SILT);
        }
        log = true;
    }

    auto create_chunk(UShortVec2 pos) {
        chunkmap.insert(pair<UShortVec2, chunk>( pos, chunk() ));
        if(log) {
            cout << "[World] -> New chunk made at " << pos.x << ", " << pos.y << " (id: " << pos.id() << ")\n";
            cout << "               Map size increased to " << pretty_size( chunkmap.size() * (sizeof(chunk) + sizeof(UShortVec2)) ) << endl;
        }
        return chunkmap.find(pos);
    }

    void set_tile(UShortVec2 rel_pos, UShortVec2 c_pos, unsigned short id) {
        auto c = chunkmap.find(c_pos);
        if(c == chunkmap.end()) {
            c = create_chunk(c_pos);
        }
        c->second.content[rel_pos.x][rel_pos.y] = id;
    }

    unsigned short create_tile(UShortVec2 rel_pos, UShortVec2 c_pos) {
        unsigned short id = 0;
        float r = float(rand())/float(RAND_MAX);

        if(r < 0.998)
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

    unsigned short unsafe_get_tile(IntVec2 pos) {
        if(pos.x<0 || pos.y<0 || pos.x>WORLD_SIZE || pos.y>WORLD_SIZE)
            return 0;

        auto c = chunkmap.find( (UShortVec2){ 
            (unsigned short)(pos.x/16), 
            (unsigned short)(pos.y/16) 
        } );

        if(c == chunkmap.end() || c->second[pos.x%16][pos.y%16] == 0)
            return 0;

        return c->second[pos.x%16][pos.y%16];
    }

    // How many extra tiles to render
    //                      left  right  top  bottom
    Vector4 r_padding = {  2,     2,    2,    9};

    void render(_player * player, float size, float scale) {
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

        float r, distance;
        unsigned char brightness;

        for(int y = (-tileh/2) - r_padding.z; y < (tileh/2) + r_padding.w; ++y) {
            for(int x = (-tilew/2) - r_padding.x; x < (tilew/2) + r_padding.y; ++x) {
                brightness = 255;
                r = PI - atan2(x, y);
                distance = dist((Vector2){0, 0.75}, (Vector2){float(x), float(y)});

                tile = get_tile({ (tilex + x), (tiley + y) });

                if (LIMIT_LIGHTING && distance > 15) {
                    brightness = 255 - (DARKNESS*2);
                    if(tiles::is_air(tile) && brightness > 255 - (DARKNESS*2) && round(distance) == 15) 
                        brightness = 255 - DARKNESS;
                }
                else {
                    for(float d = 0; d < distance; d+=0.45) {
                        if( !tiles::is_air(get_tile({ (int)(tilex + sin(r)*d + 0.5), (int)(tiley - cos(r)*d + 1) })) ) {
                            brightness-=DARKNESS;
                            if(brightness <= 255 - (DARKNESS*2))
                                break;
                        }
                    }
                }

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
                else if(brightness > 255 - (DARKNESS*2))
                    brightness = 265 - (DARKNESS*2);

                tiles::draw_tile( 
                    tile,
                    {(x * size) - (modx * size), (y * size) - (mody * size)},
                    scale,
                    (Color){brightness, brightness, brightness, 255},
                    wall,
                    i,
                    GetMousePosition().x - GetRenderWidth()/2 > (x * size) - (modx * size) && GetMousePosition().x - GetRenderWidth()/2 < ((x+1) * size) - (modx * size) &&
                    GetRenderHeight()/2 - GetMousePosition().y > ((y-1) * size) - (mody * size) && GetRenderHeight()/2 - GetMousePosition().y < (y * size) - (mody * size)
                );
            }
        }
    }
};
