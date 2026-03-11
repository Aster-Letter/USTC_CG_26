// HW2_TODO: Implement the RBFWarper class
#pragma once

#include "warper.h"

namespace USTC_CG
{
class RBFWarper : public Warper
{
   public:
    RBFWarper() = default;
    virtual ~RBFWarper() = default;
    void set_radius_scale(double radius_scale);
    Point warp(const Point& point) const override;

   protected:
    void on_control_points_updated() override;

   private:
    [[nodiscard]] double basis_function(std::size_t index, double distance) const;

    Eigen::Matrix2d affine_matrix_ = Eigen::Matrix2d::Identity();
    Point affine_offset_ = Point::Zero();
    Eigen::MatrixXd radial_weights_;
    std::vector<double> radii_;
    double radius_scale_ = 1.0;
};
}  // namespace USTC_CG