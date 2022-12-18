#include <raylib.h>

#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <thread>

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

#define DARKNESS 50
#define LIMIT_LIGHTING false

#define WORLD_SIZE 500 // Max of 4000

using namespace std;


struct wire {
    IntVec2 point;
    bool powered = false;
};

struct chunk {
    tiles::tile content[16][16];
    const tiles::tile * operator []( const short x ) const {
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

    bool show_wires = false;

    map<IntVec2, wire> wiremap;
    map<UShortVec2, chunk> chunkmap;

    void place_structure(Structure s, IntVec2 pos) {
        for(int x = pos.x;x<pos.x+s.width;++x) {
            for(int y = pos.y;y<pos.y+s.height;++y) {
                set_tile(
                    (UShortVec2){(unsigned short)(x%16), (unsigned short)(y%16)},
                    (UShortVec2){(unsigned short)(x/16), (unsigned short)(y/16)},
                    (tiles::tile){s[(UShortVec2){(unsigned short)(x-pos.x), (unsigned short)(y-pos.y)}], 1500}
                );
            }
        }
    }

    void fill_circle(IntVec2 pos, int radius, tiles::tile tile, int randomize) {
        for(int x = -radius/2;x<radius/2;++x) {
            for(int y = -radius/2;y<radius/2;++y) {
                if(pos.x + x < 0 || pos.y + y < 0 || pos.x + x > WORLD_SIZE || pos.y + y > WORLD_SIZE)
                    continue;

                if(dist(pos, (IntVec2){pos.x + x, pos.y + y}) <= radius/2 && !(randomize && (rand()%randomize))) {
                    if(tiles::is_air(tile.id) || get_tile(pos).id == tiles::ID::STONE)
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

    void generate_cave(IntVec2 pos, float size, int len, tiles::tile tile) {
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
            generate_cave(pos, (rand()%(MAX_CAVE_SIZE-MIN_CAVE_SIZE))+MIN_CAVE_SIZE, (rand()%(MAX_CAVE_LEN-MIN_CAVE_LEN))+MIN_CAVE_LEN, {tiles::ID::VACUMN, 0});
            cout << "Generating World: " << round((float(i)/float(CAVE_COUNT))*1000)/10 << "%\n";
        }

        for(int i = 0;i<ORES;++i) {
            fill_circle((IntVec2){rand()%WORLD_SIZE, rand()%WORLD_SIZE}, 1 + (rand()%4), {tiles::ID::COPPER, 1200}, 2);
        }

        for(int i = 0;i<DEPOSITS;++i) {
            generate_cave((IntVec2){rand()%WORLD_SIZE, rand()%WORLD_SIZE}, 2+(rand()%4), 2+(rand()%2), {tiles::ID::SILT, 1200});
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

    void set_tile(UShortVec2 rel_pos, UShortVec2 c_pos, tiles::tile tile) {
        auto c = chunkmap.find(c_pos);
        if(c == chunkmap.end()) {
            c = create_chunk(c_pos);
        }
        c->second.content[rel_pos.x][rel_pos.y]= tile;
    }

    void set_mass(IntVec2 pos, float mass) {
        auto c = chunkmap.find((UShortVec2){(unsigned short)(pos.x/16), (unsigned short)(pos.y/16)});
        if(c == chunkmap.end())
            return;
        c->second.content[pos.x%16][pos.y%16].mass = mass;
    }

    tiles::tile create_tile(UShortVec2 rel_pos, UShortVec2 c_pos) {
        unsigned short id = 0;
        float mass = 2000;
        float r = float(rand())/float(RAND_MAX);

        if(r < 0.998)
            id = tiles::ID::STONE;
        else {
            id = tiles::ID::TITANIUM;
            mass = 1000;
        }

        set_tile(rel_pos, c_pos, (tiles::tile){id, mass});
        return (tiles::tile){id, 120};
    }

    // Tile getting operations
    tiles::tile get_tile(IntVec2 pos) {
        if(pos.x<0 || pos.y<0 || pos.x>WORLD_SIZE || pos.y>WORLD_SIZE)
            return tiles::VOID_TILE;

        auto c = chunkmap.find( (UShortVec2){ 
            (unsigned short)(pos.x/16), 
            (unsigned short)(pos.y/16) 
        } );

        // Create a new tile if the selected tile or its chunk does not exist
        if(c == chunkmap.end() || c->second[pos.x%16][pos.y%16].id == 0) {
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
    tiles::tile unsafe_get_tile(IntVec2 pos) {
        if(pos.x<0 || pos.y<0 || pos.x>WORLD_SIZE || pos.y>WORLD_SIZE)
            return tiles::VOID_TILE;

        auto c = chunkmap.find( (UShortVec2){ 
            (unsigned short)(pos.x/16), 
            (unsigned short)(pos.y/16) 
        } );

        if(c == chunkmap.end() || c->second[pos.x%16][pos.y%16].id == 0)
            return tiles::VOID_TILE;

        return c->second[pos.x%16][pos.y%16];
    }

    // Main update tile function
    static void update_tile(chunk * c, IntVec2 pos, map<UShortVec2, chunk> * chunkmap) {
        tiles::tile * tile = &c->content[pos.x%16][pos.y%16];

        // Lambdas for tile management
        auto get_neighbor = [chunkmap, pos](IntVec2 p2) {
            if(pos.x+p2.x<0 || pos.y+p2.y<0 || pos.x+p2.x>WORLD_SIZE || pos.y+p2.y>WORLD_SIZE)
                return tiles::VOID_TILE;

            auto c = chunkmap->find((UShortVec2){
                (unsigned short)((pos.x+p2.x)/16),
                (unsigned short)((pos.y+p2.y)/16)
            });

            if(c == chunkmap->end())
                return tiles::VOID_TILE;

            return c->second.content[(pos.x+p2.x)%16][(pos.y+p2.y)%16];
        };
        auto set_neighbor_mass = [chunkmap, pos](IntVec2 p2, float mass) {
            chunk * c = &chunkmap->find((UShortVec2){
                (unsigned short)((pos.x+p2.x)/16),
                (unsigned short)((pos.y+p2.y)/16)
            })->second;
            c->content[(pos.x+p2.x)%16][(pos.y+p2.y)%16].mass = mass;
        };

        IntVec2 neighbor;
        float old = tile->mass;
        switch(tile->id) {
            case tiles::ID::OXYGEN:
            case tiles::ID::VACUMN:
                if(tile->id == tiles::ID::VACUMN)
                    tile->mass = 0;

                neighbor = (IntVec2){0, -1};
                if(tiles::is_air(get_neighbor(neighbor).id) && get_neighbor(neighbor).mass > tile->mass) {
                    tile->mass = float(tile->mass + get_neighbor(neighbor).mass)/2.0f;
                    set_neighbor_mass(neighbor, get_neighbor(neighbor).mass - (tile->mass-old));
                    if(tile->id == tiles::ID::VACUMN)
                        tile->id = get_neighbor(neighbor).id;
                }
                neighbor = (IntVec2){0, 1};
                old = tile->mass;
                if(tiles::is_air(get_neighbor(neighbor).id) && get_neighbor(neighbor).mass > tile->mass) {
                    tile->mass = float((tile->mass) + get_neighbor(neighbor).mass)/2.0f;
                    set_neighbor_mass(neighbor, get_neighbor(neighbor).mass - (tile->mass-old));
                    if(tile->id == tiles::ID::VACUMN)
                        tile->id = get_neighbor(neighbor).id;
                }
                neighbor = (IntVec2){-1, 0};
                old = tile->mass;
                if(tiles::is_air(get_neighbor(neighbor).id) && get_neighbor(neighbor).mass > tile->mass) {
                    tile->mass = float((tile->mass) + get_neighbor(neighbor).mass)/2.0f;
                    set_neighbor_mass(neighbor, get_neighbor(neighbor).mass - (tile->mass-old));
                    if(tile->id == tiles::ID::VACUMN)
                        tile->id = get_neighbor(neighbor).id;
                }
                neighbor = (IntVec2){1, 0};
                old = tile->mass;
                if(tiles::is_air(get_neighbor(neighbor).id) && get_neighbor(neighbor).mass > tile->mass) {
                    tile->mass = float((tile->mass) + get_neighbor(neighbor).mass)/2.0f;
                    set_neighbor_mass(neighbor, get_neighbor(neighbor).mass - (tile->mass-old));
                    if(tile->id == tiles::ID::VACUMN)
                        tile->id = get_neighbor(neighbor).id;
                }
            default:
                return;
        }
    }

    // Update operations
    thread updater_thread;
    static void run_updates(map<UShortVec2, chunk> * chunkmap) {
        for(auto &chunk : *chunkmap) {
            for(unsigned short x = 0;x<16;++x) {
                for(unsigned short y = 0;y<16;++y) {
                    update_tile( 
                        &chunk.second,
                        (IntVec2){
                            (chunk.first.x*16) + x,
                            (chunk.first.y*16) + y
                        },
                        chunkmap
                    );
                }
            }
        }
        return;
    }
    void tick_update() {
        if(updater_thread.joinable())
            updater_thread.join();
        updater_thread = thread(run_updates, &chunkmap);
    }
    void stop_update_thread() {
        updater_thread.join();
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

        tiles::tile tile;
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
                    if(tiles::is_transparent(tile.id) && brightness > 255 - (DARKNESS*2) && round(distance) == 15) 
                        brightness = 255 - DARKNESS;
                }
                else {
                    for(float d = 0; d < distance; d+=0.45) {
                        if(!tiles::is_transparent(get_tile({ (int)(tilex + sin(r)*d + 0.5), (int)(tiley - cos(r)*d + 1) }).id) ) {
                            brightness-=DARKNESS;
                            if(brightness <= 255 - (DARKNESS*2))
                                break;
                        }
                    }
                }

                i=0;
                if(tiles::is_air(tile.id)) {
                    wall = new short[4];
                    if(!tiles::is_air(get_tile({tilex + x + 1, tiley + y}).id)) {
                        wall[i] = 0;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x - 1, tiley + y}).id)) {
                        wall[i] = 180;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x, tiley + y + 1}).id)) {
                        wall[i] = 90;
                        ++i;
                    }
                    if(!tiles::is_air(get_tile({tilex + x, tiley + y - 1}).id)) {
                        wall[i] = 270;
                        ++i;
                    }
                }
                else if(brightness > 255 - (DARKNESS*2))
                    brightness = 265 - (DARKNESS*2);

                if(tiles::has_wire_output(tile.id)) {
                    auto welem = wiremap.find((IntVec2){ (tilex + x), (tiley + y) });
                    if(welem != wiremap.end()) {
                        wire w = welem->second;
                        BeginTextureMode(tiles::shading_buffer);

                        DrawLineEx((Vector2){(x * size) - (modx * size), (y * size) - (mody * size)}, 
                        (Vector2){(w.point.x * size) - (modx * size), (w.point.y * size) - (mody * size)},
                        4, w.powered ? GREEN : RED);

                        EndTextureMode();
                    }
                        
                }

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

        DrawText(("Mass: " + to_string((int)round(get_tile(player->select).mass))).c_str(), GetMouseX(), GetMouseY(), 10, WHITE);
    }
};
