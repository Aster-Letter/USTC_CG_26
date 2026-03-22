#include "ellipse.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
void Ellipse::draw(const Config& config) const
{
    const float rx = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float ry = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    if (rx <= 0.0f || ry <= 0.0f)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 center(
        config.bias[0] +
            (start_point_x_ + end_point_x_) * 0.5f * config.scale,
        config.bias[1] +
            (start_point_y_ + end_point_y_) * 0.5f * config.scale);
    const ImVec2 radius(rx * config.scale, ry * config.scale);

    draw_list->AddEllipse(
        center,
        radius,
        IM_COL32(
            config.line_color[0],
            config.line_color[1],
            config.line_color[2],
            config.line_color[3]),
        0.0f,
        0,
        config.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

std::vector<std::pair<int, int>> Ellipse::get_interior_pixels() const
{
    float cx = (start_point_x_ + end_point_x_) * 0.5f;
    float cy = (start_point_y_ + end_point_y_) * 0.5f;
    float rx = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    float ry = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    if (rx <= 0.0f || ry <= 0.0f)
        return {};

    int min_x = static_cast<int>(std::floor(cx - rx));
    int max_x = static_cast<int>(std::ceil(cx + rx));
    int min_y = static_cast<int>(std::floor(cy - ry));
    int max_y = static_cast<int>(std::ceil(cy + ry));

    std::vector<std::pair<int, int>> pixels;
    pixels.reserve(
        static_cast<size_t>((max_x - min_x + 1)) *
        static_cast<size_t>((max_y - min_y + 1)));

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            float dx = (static_cast<float>(x) - cx) / rx;
            float dy = (static_cast<float>(y) - cy) / ry;
            if (dx * dx + dy * dy <= 1.0f)
                pixels.emplace_back(x, y);
        }
    }
    return pixels;
}

}  // namespace USTC_CG
