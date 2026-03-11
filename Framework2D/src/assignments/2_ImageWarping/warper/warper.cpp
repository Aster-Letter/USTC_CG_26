#include "warper.h"

namespace USTC_CG
{
void Warper::set_control_points(
	const std::vector<Point>& source_points,
	const std::vector<Point>& target_points)
{
	source_points_ = source_points;
	target_points_ = target_points;
	on_control_points_updated();
}

bool Warper::has_valid_control_points() const
{
	return !source_points_.empty() &&
		   source_points_.size() == target_points_.size();
}

std::size_t Warper::control_point_count() const
{
	return source_points_.size();
}

const std::vector<Warper::Point>& Warper::source_points() const
{
	return source_points_;
}

const std::vector<Warper::Point>& Warper::target_points() const
{
	return target_points_;
}

void Warper::on_control_points_updated()
{
}
}  // namespace USTC_CG
