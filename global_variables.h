#ifndef TYPES_H
#define TYPES_H
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <utility>
#include <filesystem>

struct Point
{
    float x;
    float y;
};

struct Pixel
{
    int x;
    int y;
};

struct Map
{
    int w;
    int h;
    const char* map;
};

#endif // TYPES_H