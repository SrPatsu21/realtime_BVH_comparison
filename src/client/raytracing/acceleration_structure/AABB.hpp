#pragma once
#include <algorithm>
#include <cmath>

class AABB
{
public:

    class Axis {
    public:
        static constexpr int X = 0;
        static constexpr int Y = 1;
        static constexpr int Z = 2;
    };

    float min[3];
    float max[3];

    AABB();

    void expand(const AABB& b);
    void expand(const float p[3]);

    float surfaceArea() const;

    float getCentroidAxis(int axis) const;
};