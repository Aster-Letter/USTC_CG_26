#include "line.h"

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

// Draw the line using ImGui
void Line::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const auto& style = shape_config();

    draw_list->AddLine(
        ImVec2(
            config.bias[0] + start_point_x_, config.bias[1] + start_point_y_),
        ImVec2(config.bias[0] + end_point_x_, config.bias[1] + end_point_y_),
        to_imcolor(style.line_color),
        style.line_thickness);
}

void Line::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

bool Line::hit_test(float x, float y, float tolerance) const
{
    const auto& style = shape_config();
    return point_to_segment_distance(
               x,
               y,
               start_point_x_,
               start_point_y_,
               end_point_x_,
               end_point_y_) <=
           tolerance + style.line_thickness * 0.5f;
}
}  // namespace USTC_CG