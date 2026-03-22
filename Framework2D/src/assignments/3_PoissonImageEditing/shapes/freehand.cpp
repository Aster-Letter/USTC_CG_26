#include "freehand.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace USTC_CG
{
void Freehand::draw(const Config& config) const
{
    if (points_.size() < 2)
        return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    std::vector<ImVec2> screen_points;
    screen_points.reserve(points_.size());
    for (const auto& p : points_)
    {
        screen_points.emplace_back(
            config.bias[0] + p.x * config.scale,
            config.bias[1] + p.y * config.scale);
    }

    draw_list->AddPolyline(
        screen_points.data(),
        static_cast<int>(screen_points.size()),
        IM_COL32(
            config.line_color[0],
            config.line_color[1],
            config.line_color[2],
            config.line_color[3]),
        ImDrawFlags_Closed,
        config.line_thickness);
}

void Freehand::update(float x, float y)
{
    add_control_point(x, y);
}

void Freehand::add_control_point(float x, float y)
{
    if (!points_.empty())
    {
        const ImVec2& last = points_.back();
        float dx = x - last.x;
        float dy = y - last.y;
        if (dx * dx + dy * dy < 4.0f)
            return;
    }
    points_.push_back(ImVec2(x, y));
}

std::vector<std::pair<int, int>> Freehand::get_interior_pixels() const
{
    if (points_.size() < 3)
        return {};

    // Compute bounding box
    float min_x = points_[0].x, max_x = points_[0].x;
    float min_y = points_[0].y, max_y = points_[0].y;
    for (const auto& p : points_)
    {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }

    int iy_min = static_cast<int>(std::floor(min_y));
    int iy_max = static_cast<int>(std::ceil(max_y));
    int ix_min = static_cast<int>(std::floor(min_x));
    int ix_max = static_cast<int>(std::ceil(max_x));

    std::vector<std::pair<int, int>> pixels;
    pixels.reserve(
        static_cast<size_t>(ix_max - ix_min + 1) *
        static_cast<size_t>(iy_max - iy_min + 1));

    // Scanline fill using even-odd rule
    const int n = static_cast<int>(points_.size());
    for (int iy = iy_min; iy <= iy_max; ++iy)
    {
        float y = static_cast<float>(iy) + 0.5f;
        std::vector<float> intersections;
        for (int i = 0; i < n; ++i)
        {
            int j = (i + 1) % n;
            float y0 = points_[i].y, y1 = points_[j].y;
            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y))
            {
                float t = (y - y0) / (y1 - y0);
                float x_intersect =
                    points_[i].x + t * (points_[j].x - points_[i].x);
                intersections.push_back(x_intersect);
            }
        }
        std::sort(intersections.begin(), intersections.end());

        for (size_t k = 0; k + 1 < intersections.size(); k += 2)
        {
            int x0 = static_cast<int>(std::ceil(intersections[k]));
            int x1 = static_cast<int>(std::floor(intersections[k + 1]));
            for (int ix = x0; ix <= x1; ++ix)
                pixels.emplace_back(ix, iy);
        }
    }
    return pixels;
}

}  // namespace USTC_CG
