#pragma once

#include "Math/Vector2.hpp"
#include <vector>2

namespace bl {

class VideoMode {
public:
    VideoMode() = default;
    VideoMode(Extent2D resolution, unsigned int bitsPerPixel, unsigned int refreshRate);
    ~VideoMode() = default;

    Extent2D resolution;
    unsigned int bitsPerPixel;
    unsigned int refreshRate;
};

};