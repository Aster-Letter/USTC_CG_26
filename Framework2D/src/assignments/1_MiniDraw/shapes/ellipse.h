#pragma once

#include "shape.h"

namespace USTC_CG
{
class Ellipse : public Shape
{
   public:
    Ellipse() = default;

    Ellipse(
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

    virtual ~Ellipse() = default;

    // Overrides draw function to implement ellipse-specific drawing logic
    void draw(const Config& config) const override;
    void update(float x, float y) override;
    std::shared_ptr<Shape> clone() const override;
    bool hit_test(float x, float y, float tolerance) const override;
    void translate(float dx, float dy) override;
    void scale(float scale_x, float scale_y, const ImVec2& anchor) override;
    void rotate(float radians, const ImVec2& anchor) override;
    ImVec2 center() const override;
    ImVec2 bounds_min() const override;
    ImVec2 bounds_max() const override;
    float rotation_radians() const override;
    void set_rotation_radians(float radians) override;
    const char* type_name() const override;
    bool supports_fill() const override;

   private:
    float start_point_x_ = 0.0f;
    float start_point_y_ = 0.0f;
    float end_point_x_ = 0.0f;
    float end_point_y_ = 0.0f;
    float rotation_radians_ = 0.0f;
};
}  // namespace USTC_CG