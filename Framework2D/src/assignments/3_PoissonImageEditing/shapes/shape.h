#pragma once

#include <utility>
#include <vector>

namespace USTC_CG
{
class Shape
{
   public:
    struct Config
    {
        float bias[2] = { 0.f, 0.f };
        float scale = 1.0f;
        unsigned char line_color[4] = { 255, 0, 0, 255 };
        float line_thickness = 2.0f;
    };

   public:
    virtual ~Shape() = default;

    virtual void draw(const Config& config) const = 0;
    virtual void update(float x, float y) = 0;
    virtual void add_control_point(float x, float y)
    {
        static_cast<void>(x);
        static_cast<void>(y);
    }
    virtual std::vector<std::pair<int, int>> get_interior_pixels() const
    {
        return {};
    }
};
}  // namespace USTC_CG