#include "line.h"

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

std::shared_ptr<Shape> Line::clone() const
{
    return std::make_shared<Line>(*this);
}

bool Line::hit_test(float x, float y, float tolerance) const
{
    return point_to_segment_distance(
               ImVec2(x, y),
               ImVec2(start_point_x_, start_point_y_),
               ImVec2(end_point_x_, end_point_y_)) <=
           tolerance + config().line_thickness * 0.5f;
}

void Line::translate(float dx, float dy)
{
    start_point_x_ += dx;
    start_point_y_ += dy;
    end_point_x_ += dx;
    end_point_y_ += dy;
}

void Line::scale(float scale_x, float scale_y, const ImVec2& anchor)
{
    const ImVec2 start = scale_point(
        ImVec2(start_point_x_, start_point_y_), anchor, scale_x, scale_y);
    const ImVec2 end = scale_point(
        ImVec2(end_point_x_, end_point_y_), anchor, scale_x, scale_y);
    start_point_x_ = start.x;
    start_point_y_ = start.y;
    end_point_x_ = end.x;
    end_point_y_ = end.y;
}

void Line::rotate(float radians, const ImVec2& anchor)
{
    const ImVec2 start =
        rotate_point(ImVec2(start_point_x_, start_point_y_), anchor, radians);
    const ImVec2 end =
        rotate_point(ImVec2(end_point_x_, end_point_y_), anchor, radians);
    start_point_x_ = start.x;
    start_point_y_ = start.y;
    end_point_x_ = end.x;
    end_point_y_ = end.y;
    rotation_radians_ += radians;
}

ImVec2 Line::center() const
{
    return ImVec2(
        (start_point_x_ + end_point_x_) * 0.5f,
        (start_point_y_ + end_point_y_) * 0.5f);
}

ImVec2 Line::bounds_min() const
{
    return ImVec2(
        std::min(start_point_x_, end_point_x_),
        std::min(start_point_y_, end_point_y_));
}

ImVec2 Line::bounds_max() const
{
    return ImVec2(
        std::max(start_point_x_, end_point_x_),
        std::max(start_point_y_, end_point_y_));
}

float Line::rotation_radians() const
{
    return rotation_radians_;
}

void Line::set_rotation_radians(float radians)
{
    rotate(radians - rotation_radians_, center());
}

const char* Line::type_name() const
{
    return "Line";
}
}  // namespace USTC_CG