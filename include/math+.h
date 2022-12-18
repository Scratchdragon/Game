#ifndef _MATH+_H
#define _MATH+_H

#include <math.h>

float MOD_DEPTH = 100000;

// This returns the modulo of 2 numbers with an always positive result
int pos_modulo(int a, int b) {
    int ret = a%b;
    if(ret<0) {
        return b+ret;
    }
    return ret;
}

// Modulo for decimal (Does not require a pointer)
float dec_mod(float a, float b) {
    return float((int)(a * MOD_DEPTH) % (int)(b * MOD_DEPTH)) / MOD_DEPTH;
}

float clamp(float n, float lower, float upper) {
  return std::max(lower, std::min(n, upper));
}

int down_floor(float n) {
    if(n<0)
        return ceil(n);
    return floor(n);
}
#endif