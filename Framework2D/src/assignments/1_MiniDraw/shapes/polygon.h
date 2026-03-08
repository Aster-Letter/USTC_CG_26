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
    bool hit_test(float x, float y, float tolerance) const override;
    void add_control_point(float x, float y) override;
    bool finalize();
    bool is_empty() const { return vertices_.empty(); }

    void close() { is_closed_ = true; }

   private:
    std::vector<ImVec2> vertices_;
    bool is_closed_ = false;
};

}  // namespace USTC_CG