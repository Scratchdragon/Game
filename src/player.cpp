#pragma once

#include <raylib.h>

#include <math.h>

#include <iostream>
using namespace std;

#define PLAYER_SPEED 4.0f

class _player {
    // Non public variables
    Vector2 * window_size;

    public:

    // Collision detection
    bool collision = false;
    
    Vector2 position = {0, 0};
    float rotation = 0;

    // Rendering variables
    Texture2D sprite;
    Vector2 size = {36, 70};
    

    _player(Vector2 * window_size) {
        this->sprite = LoadTexture("resources/images/entities/player.png");
        this->window_size = window_size;
    }

    void render() {
        DrawTexturePro(
            sprite, 
            {0, 0, (float)sprite.width, (float)sprite.height}, // The original transform of the sprite
            { window_size->x / 2.0f, window_size->y / 2.0f, size.x, size.y}, // The new transform of the sprite
            {size.x/2, size.y/2}, rotation, WHITE);
    }

    void unload() {
        UnloadTexture(sprite);
    }
};