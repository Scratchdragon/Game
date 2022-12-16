#pragma once

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "../include/math+.h"

namespace Random {
    int seed = 0;

    void init(int s) {
        seed = s;
    }

    inline float Rand(long long key) {
        srand(seed + key);
        return float(rand())/float(RAND_MAX); 
    }

    inline int Int(long long key, int min, int max) { return (Rand(key)*(max-min))+min; }
    inline long Long(long long key, int min, int max) { return (Rand(key)*(max-min))+min; }
    inline double Dec(long long key, int min, int max) { return (Rand(key)*(max-min))+min; }
};