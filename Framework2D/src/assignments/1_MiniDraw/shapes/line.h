#pragma once

#include "shape.h"

namespace USTC_CG
{
class Line : public Shape
{
   public:
    Line() = default;

    // Constructor to initialize a line with start and end coordinates
    Line(
        float start_point_x,
        float start_point_y,
        float end_point_x,
        float end_point_y,
        const Config& config = Config())
        : Shape(config),
          start_point_x_(start_point_x),
          start_point_y_(start_point_y),
          end_point_x_(end_point_x),
          end_point_y_(end_point_y)
    {
    }

    virtual ~Line() = default;

    // Overrides draw function to implement line-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the end point during
    // interaction
    void update(float x, float y) override;
    bool hit_test(float x, float y, float tolerance) const override;

   private:
    float start_point_x_ = 0.0f;
    float start_point_y_ = 0.0f;
    float end_point_x_ = 0.0f;
    float end_point_y_ = 0.0f;
};
}  // namespace USTC_CG
