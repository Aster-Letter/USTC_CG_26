#pragma once

#include <vector>
#include <imgui.h>

#include "shape.h"

namespace USTC_CG
{
class Freehand : public Shape
{
   public:
    Freehand() = default;
    Freehand(float x, float y, const Config& config = Config());

    virtual ~Freehand() = default;

    void draw(const Config& config) const override;
    void update(float x, float y) override;
    bool hit_test(float x, float y, float tolerance) const override;
    void add_control_point(float x, float y) override;

   private:
    std::vector<ImVec2> points_;
};

}  // namespace USTC_CG