#include "rect.h"

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

// Draw the rectangle using ImGui
void Rect::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const auto& style = shape_config();
    const ImVec2 p_min(
        config.bias[0] + start_point_x_, config.bias[1] + start_point_y_);
    const ImVec2 p_max(
        config.bias[0] + end_point_x_, config.bias[1] + end_point_y_);

    if (style.is_filled)
    {
        draw_list->AddRectFilled(p_min, p_max, to_imcolor(style.fill_color));
    }

    draw_list->AddRect(
        p_min,
        p_max,
        to_imcolor(style.line_color),
        0.f,  // No rounding of corners
        ImDrawFlags_None,
        style.line_thickness);
}

void Rect::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

bool Rect::hit_test(float x, float y, float tolerance) const
{
    const auto& style = shape_config();
    const float min_x = std::min(start_point_x_, end_point_x_);
    const float max_x = std::max(start_point_x_, end_point_x_);
    const float min_y = std::min(start_point_y_, end_point_y_);
    const float max_y = std::max(start_point_y_, end_point_y_);

    if (style.is_filled && x >= min_x && x <= max_x && y >= min_y && y <= max_y)
    {
        return true;
    }

    const float expanded_tolerance = tolerance + style.line_thickness * 0.5f;
    return point_to_segment_distance(x, y, min_x, min_y, max_x, min_y) <=
               expanded_tolerance ||
           point_to_segment_distance(x, y, max_x, min_y, max_x, max_y) <=
               expanded_tolerance ||
           point_to_segment_distance(x, y, max_x, max_y, min_x, max_y) <=
               expanded_tolerance ||
           point_to_segment_distance(x, y, min_x, max_y, min_x, min_y) <=
               expanded_tolerance;
}

}  // namespace USTC_CG
