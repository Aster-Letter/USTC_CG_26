#include "ellipse.h"

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
            center,
            radius,
            to_imcolor(style.fill_color),
            rotation_radians_,
            0);
    }

    draw_list->AddEllipse(
        center,
        radius,
        to_imcolor(style.line_color),
        rotation_radians_,
        0,
        style.line_thickness);
}

void Ellipse::update(float x, float y)
{
    end_point_x_ = x;
    end_point_y_ = y;
}

std::shared_ptr<Shape> Ellipse::clone() const
{
    return std::make_shared<Ellipse>(*this);
}

bool Ellipse::hit_test(float x, float y, float tolerance) const
{
    const ImVec2 local = rotate_point(ImVec2(x, y), center(), -rotation_radians_);
    const float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    if (radius_x <= 1e-3f || radius_y <= 1e-3f)
    {
        return false;
    }

    const ImVec2 ellipse_center = center();
    const float normalized =
        ((local.x - ellipse_center.x) * (local.x - ellipse_center.x)) /
            ((radius_x + tolerance) * (radius_x + tolerance)) +
        ((local.y - ellipse_center.y) * (local.y - ellipse_center.y)) /
            ((radius_y + tolerance) * (radius_y + tolerance));

    if (config().is_filled)
    {
        return normalized <= 1.0f;
    }

    const float inner_radius_x = std::max(1.0f, radius_x - tolerance);
    const float inner_radius_y = std::max(1.0f, radius_y - tolerance);
    const float inner_normalized =
        ((local.x - ellipse_center.x) * (local.x - ellipse_center.x)) /
            (inner_radius_x * inner_radius_x) +
        ((local.y - ellipse_center.y) * (local.y - ellipse_center.y)) /
            (inner_radius_y * inner_radius_y);
    return normalized <= 1.0f && inner_normalized >= 1.0f;
}

void Ellipse::translate(float dx, float dy)
{
    start_point_x_ += dx;
    start_point_y_ += dy;
    end_point_x_ += dx;
    end_point_y_ += dy;
}

void Ellipse::scale(float scale_x, float scale_y, const ImVec2& anchor)
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

void Ellipse::rotate(float radians, const ImVec2& anchor)
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

ImVec2 Ellipse::center() const
{
    return ImVec2(
        (start_point_x_ + end_point_x_) * 0.5f,
        (start_point_y_ + end_point_y_) * 0.5f);
}

ImVec2 Ellipse::bounds_min() const
{
    const float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    const float cosine = std::cos(rotation_radians_);
    const float sine = std::sin(rotation_radians_);
    const float extent_x = std::sqrt(
        radius_x * radius_x * cosine * cosine +
        radius_y * radius_y * sine * sine);
    const float extent_y = std::sqrt(
        radius_x * radius_x * sine * sine +
        radius_y * radius_y * cosine * cosine);
    const ImVec2 ellipse_center = center();
    return ImVec2(ellipse_center.x - extent_x, ellipse_center.y - extent_y);
}

ImVec2 Ellipse::bounds_max() const
{
    const float radius_x = std::abs(end_point_x_ - start_point_x_) * 0.5f;
    const float radius_y = std::abs(end_point_y_ - start_point_y_) * 0.5f;
    const float cosine = std::cos(rotation_radians_);
    const float sine = std::sin(rotation_radians_);
    const float extent_x = std::sqrt(
        radius_x * radius_x * cosine * cosine +
        radius_y * radius_y * sine * sine);
    const float extent_y = std::sqrt(
        radius_x * radius_x * sine * sine +
        radius_y * radius_y * cosine * cosine);
    const ImVec2 ellipse_center = center();
    return ImVec2(ellipse_center.x + extent_x, ellipse_center.y + extent_y);
}

float Ellipse::rotation_radians() const
{
    return rotation_radians_;
}

void Ellipse::set_rotation_radians(float radians)
{
    rotate(radians - rotation_radians_, center());
}

const char* Ellipse::type_name() const
{
    return "Ellipse";
}

bool Ellipse::supports_fill() const
{
    return true;
}

}  // namespace USTC_CG