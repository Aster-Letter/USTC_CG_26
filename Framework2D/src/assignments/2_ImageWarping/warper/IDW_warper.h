// HW2_TODO: Implement the IDWWarper class
#pragma once

#include "warper.h"

namespace USTC_CG
{
class IDWWarper : public Warper
{
   public:
    IDWWarper() = default;
    virtual ~IDWWarper() = default;
    void set_weight_power(double weight_power);
    Point warp(const Point& point) const override;

   protected:
    void on_control_points_updated() override;

   private:
    double weight_power_ = 2.0;
    std::vector<Matrix2> local_transforms_;
};
}  // namespace USTC_CG