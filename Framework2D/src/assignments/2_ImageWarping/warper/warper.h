// HW2_TODO: Please implement the abstract class Warper
// 1. The Warper class should abstract the **mathematical mapping** involved in
// the warping problem, **independent of image**.
// 2. The Warper class should have a virtual function warp(...) to be called in
// our image warping application.
//    - You should design the inputs and outputs of warp(...) according to the
//    mathematical abstraction discussed in class.
//    - Generally, the warping map should map one input point to another place.
// 3. Subclasses of Warper, IDWWarper and RBFWarper, should implement the
// warp(...) function to perform the actual warping.
#pragma once

#include <cstddef>
#include <vector>
#include <Eigen/Dense>

namespace USTC_CG
{
class Warper
{
   protected:
    using Point = Eigen::Vector2d;
    using Matrix2 = Eigen::Matrix2d;
    static constexpr double kPointEpsilon = 1e-8;

    std::vector<Point> source_points_;
    std::vector<Point> target_points_;

    virtual void on_control_points_updated();

   public:
    virtual ~Warper() = default;

    void set_control_points(
        const std::vector<Point>& source_points,
        const std::vector<Point>& target_points);

    [[nodiscard]] bool has_valid_control_points() const;
    [[nodiscard]] std::size_t control_point_count() const;

    virtual Point warp(const Point& point) const = 0;

    [[nodiscard]] const std::vector<Point>& source_points() const;
    [[nodiscard]] const std::vector<Point>& target_points() const;

   private:
};
}  // namespace USTC_CG