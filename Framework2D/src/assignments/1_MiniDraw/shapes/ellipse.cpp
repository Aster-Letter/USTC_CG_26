#include "ellipse.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
void Ellipse::draw(const Config& config) const
{
    const float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    if (radius_x <= 0.0f || radius_y <= 0.0f)
    {
        return;
    }

    const auto& style = shape_config();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 center(
        config.bias[0] + (start_point_x_ + end_point_x_) * 0.5f,
        config.bias[1] + (start_point_y_ + end_point_y_) * 0.5f);
    const ImVec2 radius(radius_x, radius_y);

    if (style.is_filled)
    {
        draw_list->AddEllipseFilled(
            center, radius, to_imcolor(style.fill_color), 0.0f, 0);
    }

    draw_list->AddEllipse(
        center,
        radius,
        to_imcolor(style.line_color),
        0.f,
        0,
        style.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

bool Ellipse::hit_test(float x, float y, float tolerance) const
{
    const auto& style = shape_config();
    const float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    if (radius_x <= 1e-6f || radius_y <= 1e-6f)
    {
        return false;
    }

    const float center_x = (start_point_x_ + end_point_x_) * 0.5f;
    const float center_y = (start_point_y_ + end_point_y_) * 0.5f;
    const float dx = x - center_x;
    const float dy = y - center_y;
    const float normalized = std::sqrt(
        (dx * dx) / (radius_x * radius_x) +
        (dy * dy) / (radius_y * radius_y));

    if (style.is_filled && normalized <= 1.0f)
    {
        return true;
    }

    const float radial_tolerance =
        (tolerance + style.line_thickness * 0.5f) / std::max(1.0f, std::min(radius_x, radius_y));
    return std::abs(normalized - 1.0f) <= radial_tolerance;
}

}  // namespace USTC_CG