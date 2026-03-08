#include "freehand.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
namespace
{
float point_to_segment_distance(
    float px,
    float py,
    float ax,
    float ay,
    float bx,
    float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float ab_len_sq = abx * abx + aby * aby;
    if (ab_len_sq <= 1e-6f)
    {
        const float dx = px - ax;
        const float dy = py - ay;
        return std::sqrt(dx * dx + dy * dy);
    }

    const float t = std::clamp(
        ((px - ax) * abx + (py - ay) * aby) / ab_len_sq, 0.0f, 1.0f);
    const float cx = ax + t * abx;
    const float cy = ay + t * aby;
    const float dx = px - cx;
    const float dy = py - cy;
    return std::sqrt(dx * dx + dy * dy);
}
}

Freehand::Freehand(float x, float y, const Config& config) : Shape(config)
{
    points_.push_back(ImVec2(x, y));
}

void Freehand::draw(const Config& config) const
{
    if (points_.size() < 2)
        return;

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
        false,  // Not closed
        style.line_thickness);
}

void Freehand::update(float x, float y)
{
    add_control_point(x, y);
}

bool Freehand::hit_test(float x, float y, float tolerance) const
{
    if (points_.size() < 2)
    {
        return false;
    }

    const auto& style = shape_config();
    const float expanded_tolerance = tolerance + style.line_thickness * 0.5f;
    for (size_t i = 1; i < points_.size(); ++i)
    {
        if (point_to_segment_distance(
                x,
                y,
                points_[i - 1].x,
                points_[i - 1].y,
                points_[i].x,
                points_[i].y) <= expanded_tolerance)
        {
            return true;
        }
    }

    return false;
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
}