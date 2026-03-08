#pragma once

#include <vector>
#include <imgui.h>

#include "shape.h"

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;
    explicit Polygon(const Config& config) : Shape(config) {}

    virtual ~Polygon() = default;

    void draw(const Config& config) const override;
    void update(float x, float y) override;
    void add_control_point(float x, float y) override;
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
    bool finalize();
    bool is_empty() const { return vertices_.empty(); }

    void close() { is_closed_ = true; }

   private:
    std::vector<ImVec2> vertices_;
    bool is_closed_ = false;
    float rotation_radians_ = 0.0f;
};

}  // namespace USTC_CG