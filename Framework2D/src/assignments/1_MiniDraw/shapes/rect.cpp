#include "rect.h"

#include <algorithm>
#include <array>
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
}  // namespace

// Draw the rectangle using ImGui
void Rect::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const auto& style = shape_config();
    const ImVec2 rect_center = center();
    const float half_width = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float half_height = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    const std::array<ImVec2, 4> corners = {
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
    };
    const std::array<ImVec2, 4> screen_corners = {
        ImVec2(config.bias[0] + corners[0].x, config.bias[1] + corners[0].y),
        ImVec2(config.bias[0] + corners[1].x, config.bias[1] + corners[1].y),
        ImVec2(config.bias[0] + corners[2].x, config.bias[1] + corners[2].y),
        ImVec2(config.bias[0] + corners[3].x, config.bias[1] + corners[3].y),
    };

    if (style.is_filled)
    {
        draw_list->AddQuadFilled(
            screen_corners[0],
            screen_corners[1],
            screen_corners[2],
            screen_corners[3],
            to_imcolor(style.fill_color));
    }

    draw_list->AddQuad(
        screen_corners[0],
        screen_corners[1],
        screen_corners[2],
        screen_corners[3],
        to_imcolor(style.line_color),
        style.line_thickness);
}

void Rect::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

std::shared_ptr<Shape> Rect::clone() const
{
    return std::make_shared<Rect>(*this);
}

bool Rect::hit_test(float x, float y, float tolerance) const
{
    const ImVec2 local = rotate_point(ImVec2(x, y), center(), -rotation_radians_);
    const float min_x = std::min(start_point_x_, end_point_x_);
    const float max_x = std::max(start_point_x_, end_point_x_);
    const float min_y = std::min(start_point_y_, end_point_y_);
    const float max_y = std::max(start_point_y_, end_point_y_);

    if (config().is_filled)
    {
        return local.x >= min_x - tolerance && local.x <= max_x + tolerance &&
               local.y >= min_y - tolerance && local.y <= max_y + tolerance;
    }

    const bool near_left = std::abs(local.x - min_x) <= tolerance &&
                           local.y >= min_y - tolerance &&
                           local.y <= max_y + tolerance;
    const bool near_right = std::abs(local.x - max_x) <= tolerance &&
                            local.y >= min_y - tolerance &&
                            local.y <= max_y + tolerance;
    const bool near_top = std::abs(local.y - min_y) <= tolerance &&
                          local.x >= min_x - tolerance &&
                          local.x <= max_x + tolerance;
    const bool near_bottom = std::abs(local.y - max_y) <= tolerance &&
                             local.x >= min_x - tolerance &&
                             local.x <= max_x + tolerance;
    return near_left || near_right || near_top || near_bottom;
}

void Rect::translate(float dx, float dy)
{
    start_point_x_ += dx;
    end_point_x_ += dx;
    start_point_y_ += dy;
    end_point_y_ += dy;
}

void Rect::scale(float scale_x, float scale_y, const ImVec2& anchor)
{
    const ImVec2 scaled_center = scale_point(center(), anchor, scale_x, scale_y);
    const float half_width =
        std::max(1.0f, std::abs(end_point_x_ - start_point_x_) * 0.5f * scale_x);
    const float half_height = std::max(
        1.0f, std::abs(end_point_y_ - start_point_y_) * 0.5f * scale_y);
    start_point_x_ = scaled_center.x - half_width;
    end_point_x_ = scaled_center.x + half_width;
    start_point_y_ = scaled_center.y - half_height;
    end_point_y_ = scaled_center.y + half_height;
}

void Rect::rotate(float radians, const ImVec2& anchor)
{
    const ImVec2 rotated_center = rotate_point(center(), anchor, radians);
    const float half_width = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float half_height = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    start_point_x_ = rotated_center.x - half_width;
    end_point_x_ = rotated_center.x + half_width;
    start_point_y_ = rotated_center.y - half_height;
    end_point_y_ = rotated_center.y + half_height;
    rotation_radians_ += radians;
}

ImVec2 Rect::center() const
{
    return ImVec2(
        (start_point_x_ + end_point_x_) * 0.5f,
        (start_point_y_ + end_point_y_) * 0.5f);
}

ImVec2 Rect::bounds_min() const
{
    const ImVec2 rect_center = center();
    const float half_width = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float half_height = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    const std::array<ImVec2, 4> corners = {
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
    };

    ImVec2 result = corners[0];
    for (const auto& corner : corners)
    {
        result.x = std::min(result.x, corner.x);
        result.y = std::min(result.y, corner.y);
    }
    return result;
}

ImVec2 Rect::bounds_max() const
{
    const ImVec2 rect_center = center();
    const float half_width = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float half_height = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    const std::array<ImVec2, 4> corners = {
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y - half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x + half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
        rotate_point(
            ImVec2(rect_center.x - half_width, rect_center.y + half_height),
            rect_center,
            rotation_radians_),
    };

    ImVec2 result = corners[0];
    for (const auto& corner : corners)
    {
        result.x = std::max(result.x, corner.x);
        result.y = std::max(result.y, corner.y);
    }
    return result;
}

float Rect::rotation_radians() const
{
    return rotation_radians_;
}

void Rect::set_rotation_radians(float radians)
{
    rotate(radians - rotation_radians_, center());
}

const char* Rect::type_name() const
{
    return "Rectangle";
}

bool Rect::supports_fill() const
{
    return true;
}

}  // namespace USTC_CG
