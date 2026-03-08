#include "polygon.h"

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

bool point_in_polygon(const ImVec2& point, const std::vector<ImVec2>& vertices)
{
    bool inside = false;
    for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++)
    {
        const ImVec2& vi = vertices[i];
        const ImVec2& vj = vertices[j];
        const bool intersects =
            ((vi.y > point.y) != (vj.y > point.y)) &&
            (point.x <
             (vj.x - vi.x) * (point.y - vi.y) / ((vj.y - vi.y) + 1e-6f) + vi.x);
        if (intersects)
        {
            inside = !inside;
        }
    }
    return inside;
}
}  // namespace

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

std::shared_ptr<Shape> Polygon::clone() const
{
    return std::make_shared<Polygon>(*this);
}

bool Polygon::hit_test(float x, float y, float tolerance) const
{
    if (vertices_.size() < 2)
    {
        return false;
    }

    const ImVec2 point(x, y);
    for (size_t i = 1; i < vertices_.size(); ++i)
    {
        if (point_to_segment_distance(point, vertices_[i - 1], vertices_[i]) <=
            tolerance + config().line_thickness * 0.5f)
        {
            return true;
        }
    }

    if (is_closed_ &&
        point_to_segment_distance(point, vertices_.back(), vertices_.front()) <=
            tolerance + config().line_thickness * 0.5f)
    {
        return true;
    }

    return config().is_filled && is_closed_ && point_in_polygon(point, vertices_);
}

void Polygon::translate(float dx, float dy)
{
    for (auto& vertex : vertices_)
    {
        vertex.x += dx;
        vertex.y += dy;
    }
}

void Polygon::scale(float scale_x, float scale_y, const ImVec2& anchor)
{
    for (auto& vertex : vertices_)
    {
        vertex = scale_point(vertex, anchor, scale_x, scale_y);
    }
}

void Polygon::rotate(float radians, const ImVec2& anchor)
{
    for (auto& vertex : vertices_)
    {
        vertex = rotate_point(vertex, anchor, radians);
    }
    rotation_radians_ += radians;
}

ImVec2 Polygon::center() const
{
    const ImVec2 min_point = bounds_min();
    const ImVec2 max_point = bounds_max();
    return ImVec2(
        (min_point.x + max_point.x) * 0.5f,
        (min_point.y + max_point.y) * 0.5f);
}

ImVec2 Polygon::bounds_min() const
{
    if (vertices_.empty())
    {
        return ImVec2(0.0f, 0.0f);
    }

    ImVec2 result = vertices_.front();
    for (const auto& vertex : vertices_)
    {
        result.x = std::min(result.x, vertex.x);
        result.y = std::min(result.y, vertex.y);
    }
    return result;
}

ImVec2 Polygon::bounds_max() const
{
    if (vertices_.empty())
    {
        return ImVec2(0.0f, 0.0f);
    }

    ImVec2 result = vertices_.front();
    for (const auto& vertex : vertices_)
    {
        result.x = std::max(result.x, vertex.x);
        result.y = std::max(result.y, vertex.y);
    }
    return result;
}

float Polygon::rotation_radians() const
{
    return rotation_radians_;
}

void Polygon::set_rotation_radians(float radians)
{
    rotate(radians - rotation_radians_, center());
}

const char* Polygon::type_name() const
{
    return "Polygon";
}

bool Polygon::supports_fill() const
{
    return true;
}
}  // namespace USTC_CG