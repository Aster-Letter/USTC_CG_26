#include "freehand.h"

#include <algorithm>
#include <cmath>

#include <imgui.h>

namespace USTC_CG
{
namespace
{
ImVec2 rotate_point(const ImVec2& point, const ImVec2& anchor, float radians)
{
    const float sine = std::sin(radians);
    const float cosine = std::cos(radians);
    const float dx = point.x - anchor.x;
    const float dy = point.y - anchor.y;
    return ImVec2(
        anchor.x + dx * cosine - dy * sine,
        anchor.y + dx * sine + dy * cosine);
}

ImVec2 scale_point(
    const ImVec2& point,
    const ImVec2& anchor,
    float scale_x,
    float scale_y)
{
    return ImVec2(
        anchor.x + (point.x - anchor.x) * scale_x,
        anchor.y + (point.y - anchor.y) * scale_y);
}

float point_to_segment_distance(
    const ImVec2& point,
    const ImVec2& segment_start,
    const ImVec2& segment_end)
{
    const float dx = segment_end.x - segment_start.x;
    const float dy = segment_end.y - segment_start.y;
    const float length_squared = dx * dx + dy * dy;
    if (length_squared <= 1e-6f)
    {
        const float px = point.x - segment_start.x;
        const float py = point.y - segment_start.y;
        return std::sqrt(px * px + py * py);
    }

    const float t = std::clamp(
        ((point.x - segment_start.x) * dx +
         (point.y - segment_start.y) * dy) /
            length_squared,
        0.0f,
        1.0f);
    const float projection_x = segment_start.x + t * dx;
    const float projection_y = segment_start.y + t * dy;
    const float distance_x = point.x - projection_x;
    const float distance_y = point.y - projection_y;
    return std::sqrt(distance_x * distance_x + distance_y * distance_y);
}
}  // namespace

Freehand::Freehand(float x, float y, const Config& config) : Shape(config)
{
    points_.push_back(ImVec2(x, y));
}

void Freehand::draw(const Config& config) const
{
    if (points_.size() < 2)
    {
        return;
    }

    const auto& style = shape_config();
    std::vector<ImVec2> screen_points;
    screen_points.reserve(points_.size());
    for (const auto& p : points_)
    {
        screen_points.emplace_back(
            config.bias[0] + p.x,
            config.bias[1] + p.y);
    }

    ImGui::GetWindowDrawList()->AddPolyline(
        screen_points.data(),
        static_cast<int>(screen_points.size()),
        to_imcolor(style.line_color),
        false,
        style.line_thickness);
}

void Freehand::update(float x, float y)
{
    add_control_point(x, y);
}

void Freehand::add_control_point(float x, float y)
{
    if (points_.empty())
    {
        points_.push_back(ImVec2(x, y));
        return;
    }

    const ImVec2& last = points_.back();
    const float dx = x - last.x;
    const float dy = y - last.y;

    if (dx * dx + dy * dy >= 4.0f)
    {
        points_.push_back(ImVec2(x, y));
    }
}

std::shared_ptr<Shape> Freehand::clone() const
{
    return std::make_shared<Freehand>(*this);
}

bool Freehand::hit_test(float x, float y, float tolerance) const
{
    if (points_.size() < 2)
    {
        return false;
    }

    const ImVec2 point(x, y);
    for (size_t i = 1; i < points_.size(); ++i)
    {
        if (point_to_segment_distance(point, points_[i - 1], points_[i]) <=
            tolerance + config().line_thickness * 0.5f)
        {
            return true;
        }
    }
    return false;
}

void Freehand::translate(float dx, float dy)
{
    for (auto& point : points_)
    {
        point.x += dx;
        point.y += dy;
    }
}

void Freehand::scale(float scale_x, float scale_y, const ImVec2& anchor)
{
    for (auto& point : points_)
    {
        point = scale_point(point, anchor, scale_x, scale_y);
    }
}

void Freehand::rotate(float radians, const ImVec2& anchor)
{
    for (auto& point : points_)
    {
        point = rotate_point(point, anchor, radians);
    }
    rotation_radians_ += radians;
}

ImVec2 Freehand::center() const
{
    const ImVec2 min_point = bounds_min();
    const ImVec2 max_point = bounds_max();
    return ImVec2(
        (min_point.x + max_point.x) * 0.5f,
        (min_point.y + max_point.y) * 0.5f);
}

ImVec2 Freehand::bounds_min() const
{
    if (points_.empty())
    {
        return ImVec2(0.0f, 0.0f);
    }

    ImVec2 result = points_.front();
    for (const auto& point : points_)
    {
        result.x = std::min(result.x, point.x);
        result.y = std::min(result.y, point.y);
    }
    return result;
}

ImVec2 Freehand::bounds_max() const
{
    if (points_.empty())
    {
        return ImVec2(0.0f, 0.0f);
    }

    ImVec2 result = points_.front();
    for (const auto& point : points_)
    {
        result.x = std::max(result.x, point.x);
        result.y = std::max(result.y, point.y);
    }
    return result;
}

float Freehand::rotation_radians() const
{
    return rotation_radians_;
}

void Freehand::set_rotation_radians(float radians)
{
    rotate(radians - rotation_radians_, center());
}

const char* Freehand::type_name() const
{
    return "Freehand";
}
}  // namespace USTC_CG