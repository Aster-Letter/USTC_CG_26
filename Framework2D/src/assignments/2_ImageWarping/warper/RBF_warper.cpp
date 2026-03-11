#include "RBF_warper.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace USTC_CG
{
namespace
{
constexpr double kEpsilon = 1e-8;
}

void RBFWarper::set_radius_scale(double radius_scale)
{
	radius_scale_ = std::max(radius_scale, kEpsilon);
}

void RBFWarper::on_control_points_updated()
{
	affine_matrix_ = Matrix2::Identity();
	affine_offset_ = Point::Zero();
	radial_weights_.resize(0, 0);
	radii_.clear();

	if (!has_valid_control_points())
	{
		return;
	}

	const std::size_t count = control_point_count();
	radii_.assign(count, 1.0);

	for (std::size_t i = 0; i < count; ++i)
	{
		double min_distance = std::numeric_limits<double>::max();
		for (std::size_t j = 0; j < count; ++j)
		{
			if (i == j)
			{
				continue;
			}

			min_distance = std::min(
				min_distance,
				(source_points()[i] - source_points()[j]).norm());
		}

		if (min_distance < std::numeric_limits<double>::max())
		{
			radii_[i] = std::max(min_distance * radius_scale_, kEpsilon);
		}
	}

	Eigen::MatrixXd kernel_matrix(count, count);
	Eigen::MatrixXd residual_matrix(count, 2);
	for (std::size_t row = 0; row < count; ++row)
	{
		const Point affine_value = affine_matrix_ * source_points()[row] + affine_offset_;
		residual_matrix.row(static_cast<Eigen::Index>(row)) =
			(target_points()[row] - affine_value).transpose();

		for (std::size_t column = 0; column < count; ++column)
		{
			const double distance =
				(source_points()[row] - source_points()[column]).norm();
			kernel_matrix(
				static_cast<Eigen::Index>(row),
				static_cast<Eigen::Index>(column)) = basis_function(column, distance);
		}
	}

	kernel_matrix += kEpsilon * Eigen::MatrixXd::Identity(count, count);
	radial_weights_ = kernel_matrix.colPivHouseholderQr().solve(residual_matrix);
}

double RBFWarper::basis_function(std::size_t index, double distance) const
{
	const double radius = radii_.empty() ? 1.0 : radii_[index];
	return 1.0 / std::sqrt(distance * distance + radius * radius);
}

Warper::Point RBFWarper::warp(const Point& point) const
{
	if (!has_valid_control_points())
	{
		return point;
	}

	Point warped_point = affine_matrix_ * point + affine_offset_;
	for (std::size_t i = 0; i < control_point_count(); ++i)
	{
		const double distance = (point - source_points()[i]).norm();
		warped_point += radial_weights_.row(static_cast<Eigen::Index>(i)).transpose() *
						basis_function(i, distance);
	}
	return warped_point;
}
}  // namespace USTC_CG
