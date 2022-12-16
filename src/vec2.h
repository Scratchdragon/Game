#pragma once

#include <raylib.h>

// Vector2 with integer values
struct IntVec2 {
    int x, y;
    bool operator <( const IntVec2 &rhs ) const {
        // Calculate unique ID
        return ( (2^x)+y < (2^rhs.x)+rhs.y );
    }

    IntVec2 operator+( const IntVec2 &rhs ) const {
        return (IntVec2){ x+rhs.x, y+rhs.y };
    }
    IntVec2 operator-( const IntVec2 &rhs ) const {
        return (IntVec2){ x-rhs.x, y-rhs.y };
    }
    IntVec2 operator*( const IntVec2 &rhs ) const {
        return (IntVec2){ x*rhs.x, y*rhs.y };
    }
    IntVec2 operator/( const IntVec2 &rhs ) const {
        return (IntVec2){ x/rhs.x, y/rhs.y };
    }
};

// Vector2 with long values
struct LongVec2 {
    long x, y;

    LongVec2 operator+( const LongVec2 &rhs ) const {
        return (LongVec2){ x+rhs.x, y+rhs.y };
    }
    LongVec2 operator-( const LongVec2 &rhs ) const {
        return (LongVec2){ x-rhs.x, y-rhs.y };
    }
    LongVec2 operator*( const LongVec2 &rhs ) const {
        return (LongVec2){ x*rhs.x, y*rhs.y };
    }
    LongVec2 operator/( const LongVec2 &rhs ) const {
        return (LongVec2){ x/rhs.x, y/rhs.y };
    }

    long id() const {
        long x2 = abs(x);
        long y2 = abs(y);
        if (x2<y2) {
            return x2 * (y2-1) + trunc(pow(y2 - x2 - 2, 2) / 4);
        }
        else {
            return (x2 - 1) * y2 + trunc(pow(y2 - x2 - 2, 2) / 4);
        }
    } 
};

// Vector2 with long long values
struct LongLongVec2 {
    long long x,y;
    bool operator <( const LongLongVec2 &rhs ) const {
        // Calculate unique ID
        return ( (2^x)+y < (2^rhs.x)+rhs.y );
    }

    LongLongVec2 operator+( const LongLongVec2 &rhs ) const {
        return (LongLongVec2){ x+rhs.x, y+rhs.y };
    }
    LongLongVec2 operator-( const LongLongVec2 &rhs ) const {
        return (LongLongVec2){ x-rhs.x, y-rhs.y };
    }
    LongLongVec2 operator*( const LongLongVec2 &rhs ) const {
        return (LongLongVec2){ x*rhs.x, y*rhs.y };
    }
    LongLongVec2 operator/( const LongLongVec2 &rhs ) const {
        return (LongLongVec2){ x/rhs.x, y/rhs.y };
    }
};

// Vector2 with short values
struct ShortVec2 {
    short x,y;
    bool operator <( const ShortVec2 &rhs ) const {
        // Calculate unique ID
        return ( (2^x)+y < (2^rhs.x)+rhs.y );
    }
};

float dist(Vector2 a, Vector2 b) {
    return sqrt( pow(a.x - b.x, 2) + pow(a.y - b.y, 2) );
}