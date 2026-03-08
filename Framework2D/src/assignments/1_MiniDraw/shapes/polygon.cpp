#include "polygon.h"

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

bool point_in_polygon(const std::vector<ImVec2>& vertices, float x, float y)
{
    bool inside = false;
    const size_t count = vertices.size();
    for (size_t i = 0, j = count - 1; i < count; j = i++)
    {
        const ImVec2& a = vertices[i];
        const ImVec2& b = vertices[j];
        const bool intersects = ((a.y > y) != (b.y > y)) &&
                                (x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x);
        if (intersects)
        {
            inside = !inside;
        }
    }
    return inside;
}
}

void Polygon::draw(const Config& config) const
{
    if (vertices_.size() < 2)
        return;

    const auto& style = shape_config();
    std::vector<ImVec2> screen_points;
    screen_points.reserve(vertices_.size());
    for (const auto& p : vertices_)
    {
        screen_points.emplace_back(
            config.bias[0] + p.x,
            config.bias[1] + p.y);
    }

    if (style.is_filled && is_closed_ && screen_points.size() >= 3)
    {
        ImGui::GetWindowDrawList()->AddConcavePolyFilled(
            screen_points.data(),
            static_cast<int>(screen_points.size()),
            to_imcolor(style.fill_color));
    }

    ImGui::GetWindowDrawList()->AddPolyline(
        screen_points.data(),
        static_cast<int>(screen_points.size()),
        to_imcolor(style.line_color),
        is_closed_,
        style.line_thickness);
}

void Polygon::update(float x, float y)
{
    if (!vertices_.empty())
    {
        vertices_.back() = ImVec2(x, y);
    }
}

bool Polygon::hit_test(float x, float y, float tolerance) const
{
    if (vertices_.size() < 2)
    {
        return false;
    }

    const auto& style = shape_config();
    if (style.is_filled && is_closed_ && vertices_.size() >= 3 &&
        point_in_polygon(vertices_, x, y))
    {
        return true;
    }

    const float expanded_tolerance = tolerance + style.line_thickness * 0.5f;
    for (size_t i = 1; i < vertices_.size(); ++i)
    {
        if (point_to_segment_distance(
                x,
                y,
                vertices_[i - 1].x,
                vertices_[i - 1].y,
                vertices_[i].x,
                vertices_[i].y) <= expanded_tolerance)
        {
            return true;
        }
    }

    if (is_closed_)
    {
        return point_to_segment_distance(
                   x,
                   y,
                   vertices_.back().x,
                   vertices_.back().y,
                   vertices_.front().x,
                   vertices_.front().y) <= expanded_tolerance;
    }

    return false;
}

void Polygon::add_control_point(float x, float y)
{
    if (vertices_.empty())
    {
        vertices_.push_back(ImVec2(x, y));
        vertices_.push_back(ImVec2(x, y));
    }
    else
    {
        vertices_.back() = ImVec2(x, y);
        vertices_.push_back(ImVec2(x, y));
    }
}

bool Polygon::finalize()
{
    if (vertices_.size() < 2)
    {
        vertices_.clear();
        is_closed_ = false;
        return false;
    }

    vertices_.pop_back();
    if (vertices_.size() < 3)
    {
        vertices_.clear();
        is_closed_ = false;
        return false;
    }

    close();
    return true;
}
}  // namespace USTC_CG