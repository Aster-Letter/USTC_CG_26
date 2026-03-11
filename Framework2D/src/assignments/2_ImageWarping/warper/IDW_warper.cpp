#include "IDW_warper.h"

#include <limits>
#include <cmath>

namespace USTC_CG
{
namespace
{
constexpr double kEpsilon = 1e-8;
}

void IDWWarper::set_weight_power(double weight_power)
{
	weight_power_ = std::max(weight_power, 1.0 + kEpsilon);
}

void IDWWarper::on_control_points_updated()
{
	local_transforms_.clear();
	if (!has_valid_control_points())
	{
		return;
	}

	local_transforms_.resize(control_point_count(), Matrix2::Identity());
	if (control_point_count() <= 1)
	{
		return;
	}

	for (std::size_t i = 0; i < control_point_count(); ++i)
	{
		Matrix2 matrix_a = Matrix2::Zero();
		Matrix2 matrix_b = Matrix2::Zero();

		for (std::size_t j = 0; j < control_point_count(); ++j)
		{
			if (i == j)
			{
				continue;
			}

			const Point delta_source = source_points()[j] - source_points()[i];
			const Point delta_target = target_points()[j] - target_points()[i];
			const double distance = delta_source.norm();
			if (distance < kEpsilon)
			{
				continue;
			}

			const double sigma = 1.0 / std::pow(distance, weight_power_);
			matrix_a += sigma * (delta_source * delta_source.transpose());
			matrix_b += sigma * (delta_target * delta_source.transpose());
		}

		if (matrix_a.fullPivLu().rank() >= 2)
		{
			local_transforms_[i] = matrix_a.transpose()
				.ldlt()
				.solve(matrix_b.transpose())
				.transpose();
		}
	}
}

Warper::Point IDWWarper::warp(const Point& point) const
{
	if (!has_valid_control_points())
	{
		return point;
	}

	double total_weight = 0.0;
	Point warped_point = Point::Zero();

	for (std::size_t i = 0; i < control_point_count(); ++i)
	{
		const Point delta = point - source_points()[i];
		const double distance = delta.norm();
		if (distance < kEpsilon)
		{
			return target_points()[i];
		}

		const double weight = 1.0 / std::pow(distance, weight_power_);
		const Point local_mapping =
			target_points()[i] + local_transforms_[i] * delta;
		warped_point += weight * local_mapping;
		total_weight += weight;
	}

	if (total_weight < kEpsilon)
	{
		return point;
	}

	return warped_point / total_weight;
}
}  // namespace USTC_CG
