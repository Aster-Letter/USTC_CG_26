#pragma once

#include "shape.h"

#include <imgui.h>

namespace USTC_CG
{
class Freehand : public Shape
{
   public:
    Freehand() = default;

    Freehand(float x, float y)
    {
        points_.push_back(ImVec2(x, y));
    }

    virtual ~Freehand() = default;

    void draw(const Config& config) const override;
    void update(float x, float y) override;
    void add_control_point(float x, float y) override;
    std::vector<std::pair<int, int>> get_interior_pixels() const override;

   private:
    std::vector<ImVec2> points_;
};
}  // namespace USTC_CG
