#include "canvas_widget.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "imgui.h"
#include "shapes/ellipse.h"
#include "shapes/freehand.h"
#include "shapes/line.h"
#include "shapes/polygon.h"
#include "shapes/rect.h"

namespace USTC_CG
{
void Canvas::draw()
{
    draw_background();

    if (ImGui::GetIO().WantCaptureMouse && !is_hovered_ && !is_active_)
    {
        draw_shapes();
        return;
    }

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        mouse_click_event();
    }

    if (
        is_hovered_ && draw_status_ && shape_type_ == kPolygon &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        mouse_release_event();
    }

    mouse_move_event();

    if (
        draw_status_ && is_drag_shape(shape_type_) &&
        !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mouse_release_event();
    }

    draw_shapes();
}

Canvas::ShapeType Canvas::shape_type() const
{
    return shape_type_;
}

Shape::Config& Canvas::mutable_pending_config()
{
    return pending_config_;
}

const Shape::Config& Canvas::pending_config() const
{
    return pending_config_;
}

void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

void Canvas::set_background_color(const float color[4])
{
    std::copy(color, color + 4, background_color_);
}

const float* Canvas::background_color() const
{
    return background_color_;
}

void Canvas::set_antialiasing(bool enabled)
{
    antialiasing_ = enabled;
}

bool Canvas::antialiasing() const
{
    return antialiasing_;
}

void Canvas::set_max_undo_steps(int steps)
{
    max_undo_steps_ = std::max(1, steps);
    trim_history();
}

int Canvas::max_undo_steps() const
{
    return max_undo_steps_;
}

void Canvas::set_eraser_tolerance(float tolerance)
{
    eraser_tolerance_ = std::max(2.0f, tolerance);
}

float Canvas::eraser_tolerance() const
{
    return eraser_tolerance_;
}

ImVec2 Canvas::canvas_min() const
{
    return canvas_min_;
}

ImVec2 Canvas::canvas_size() const
{
    return canvas_size_;
}

bool Canvas::is_canvas_hovered() const
{
    return is_hovered_;
}

void Canvas::set_default()
{
    cancel_current_shape();
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    cancel_current_shape();
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    cancel_current_shape();
    shape_type_ = kRect;
}

void Canvas::set_ellipse()
{
    cancel_current_shape();
    shape_type_ = kEllipse;
}

void Canvas::set_polygon()
{
    cancel_current_shape();
    shape_type_ = kPolygon;
}

void Canvas::set_freehand()
{
    cancel_current_shape();
    shape_type_ = kFreehand;
}

void Canvas::set_eraser()
{
    cancel_current_shape();
    shape_type_ = kEraser;
}

void Canvas::clear_shape_list()
{
    if (shape_list_.empty())
    {
        cancel_current_shape();
        return;
    }

    save_undo_state();
    cancel_current_shape();
    shape_list_.clear();
}

void Canvas::undo()
{
    cancel_current_shape();
    if (undo_history_.empty())
    {
        return;
    }

    redo_history_.push_back(shape_list_);
    shape_list_ = undo_history_.back();
    undo_history_.pop_back();
}

void Canvas::redo()
{
    cancel_current_shape();
    if (redo_history_.empty())
    {
        return;
    }

    undo_history_.push_back(shape_list_);
    shape_list_ = redo_history_.back();
    redo_history_.pop_back();
    while (static_cast<int>(undo_history_.size()) > max_undo_steps_)
    {
        undo_history_.erase(undo_history_.begin());
    }
}

bool Canvas::can_undo() const
{
    return !undo_history_.empty();
}

bool Canvas::can_redo() const
{
    return !redo_history_.empty();
}

bool Canvas::has_shapes() const
{
    return !shape_list_.empty();
}

void Canvas::draw_background()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (show_background_)
    {
        // Draw background recrangle
        draw_list->AddRectFilled(
            canvas_min_,
            canvas_max_,
            ImGui::ColorConvertFloat4ToU32(ImVec4(
                background_color_[0],
                background_color_[1],
                background_color_[2],
                background_color_[3])));
        // Draw background border
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    Shape::Config s;
    s.bias[0] = canvas_min_.x;
    s.bias[1] = canvas_min_.y;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImDrawListFlags previous_flags = draw_list->Flags;

    if (antialiasing_)
    {
        draw_list->Flags |=
            ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill;
    }
    else
    {
        draw_list->Flags &=
            ~(ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill);
    }

    // ClipRect can hide the drawing content outside of the rectangular area
    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(s);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(s);
    }
    draw_list->PopClipRect();
    draw_list->Flags = previous_flags;

    draw_eraser_overlay();
}

void Canvas::mouse_click_event()
{
    start_point_ = end_point_ = mouse_pos_in_canvas();

    if (shape_type_ == USTC_CG::Canvas::kEraser)
    {
        erase_shape_at(start_point_.x, start_point_.y);
        return;
    }

    if (shape_type_ == USTC_CG::Canvas::kPolygon)
    {
        if (!draw_status_)
        {
            draw_status_ = true;
            current_shape_ = std::make_shared<Polygon>(pending_config_);
        }

        if (current_shape_)
        {
            current_shape_->add_control_point(start_point_.x, start_point_.y);
        }
        return;
    }

    if (draw_status_)
    {
        return;
    }

    draw_status_ = true;
    switch (shape_type_)
    {
        case USTC_CG::Canvas::kDefault:
        {
            draw_status_ = false;
            break;
        }
        case USTC_CG::Canvas::kLine:
        {
            current_shape_ = std::make_shared<Line>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        }
        case USTC_CG::Canvas::kRect:
        {
            current_shape_ = std::make_shared<Rect>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        }
        case USTC_CG::Canvas::kEllipse:
        {
            current_shape_ = std::make_shared<Ellipse>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        }
        case USTC_CG::Canvas::kFreehand:
        {
            current_shape_ = std::make_shared<Freehand>(
                start_point_.x, start_point_.y, pending_config_);
            break;
        }
        default:
        {
            draw_status_ = false;
            break;
        }
    }
}

void Canvas::mouse_move_event()
{
    if (
        shape_type_ == kEraser && is_hovered_ &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        end_point_ = mouse_pos_in_canvas();
        erase_shape_at(end_point_.x, end_point_.y);
        return;
    }

    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (current_shape_)
        {
            current_shape_->update(end_point_.x, end_point_.y);
        }
    }
}

void Canvas::mouse_release_event()
{
    if (!draw_status_ || !current_shape_)
    {
        return;
    }

    bool should_store_shape = true;
    if (shape_type_ == kPolygon)
    {
        auto polygon = std::dynamic_pointer_cast<Polygon>(current_shape_);
        should_store_shape = polygon && polygon->finalize();
    }
    else if (shape_type_ == kFreehand)
    {
        current_shape_->add_control_point(end_point_.x, end_point_.y);
        should_store_shape = std::abs(end_point_.x - start_point_.x) > 1.0f ||
                             std::abs(end_point_.y - start_point_.y) > 1.0f;
    }
    else
    {
        should_store_shape = std::abs(end_point_.x - start_point_.x) > 1.0f ||
                             std::abs(end_point_.y - start_point_.y) > 1.0f;
    }

    if (should_store_shape)
    {
        commit_shape(current_shape_);
    }

    cancel_current_shape();
}

ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}

void Canvas::commit_shape(const std::shared_ptr<Shape>& shape)
{
    if (!shape)
    {
        return;
    }

    save_undo_state();
    shape_list_.push_back(shape);
    trim_history();
}

void Canvas::cancel_current_shape()
{
    current_shape_.reset();
    draw_status_ = false;
}

bool Canvas::erase_shape_at(float x, float y)
{
    for (auto it = shape_list_.rbegin(); it != shape_list_.rend(); ++it)
    {
        if ((*it)->hit_test(x, y, eraser_tolerance_))
        {
            save_undo_state();
            shape_list_.erase(std::next(it).base());
            return true;
        }
    }

    return false;
}

void Canvas::save_undo_state()
{
    undo_history_.push_back(shape_list_);
    redo_history_.clear();
    while (static_cast<int>(undo_history_.size()) > max_undo_steps_)
    {
        undo_history_.erase(undo_history_.begin());
    }
}

void Canvas::draw_eraser_overlay()
{
    if (shape_type_ != kEraser || !is_hovered_)
    {
        return;
    }

    const ImVec2 mouse = mouse_pos_in_canvas();
    if (
        mouse.x < 0.0f || mouse.y < 0.0f || mouse.x > canvas_size_.x ||
        mouse.y > canvas_size_.y)
    {
        return;
    }

    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    const ImVec2 center(canvas_min_.x + mouse.x, canvas_min_.y + mouse.y);
    const float outer_radius = eraser_tolerance_;
    const float inner_radius = std::max(outer_radius - 3.0f, 2.0f);

    draw_list->AddCircleFilled(
        center,
        outer_radius,
        IM_COL32(255, 80, 80, 32),
        48);
    draw_list->AddCircle(
        center,
        outer_radius,
        IM_COL32(255, 120, 120, 220),
        48,
        2.0f);
    draw_list->AddCircle(
        center,
        inner_radius,
        IM_COL32(255, 255, 255, 180),
        48,
        1.0f);
    draw_list->AddLine(
        ImVec2(center.x - 4.0f, center.y),
        ImVec2(center.x + 4.0f, center.y),
        IM_COL32(255, 255, 255, 220),
        1.0f);
    draw_list->AddLine(
        ImVec2(center.x, center.y - 4.0f),
        ImVec2(center.x, center.y + 4.0f),
        IM_COL32(255, 255, 255, 220),
        1.0f);
}

bool Canvas::is_drag_shape(ShapeType type) const
{
    return type == kLine || type == kRect || type == kEllipse ||
           type == kFreehand;
}

bool Canvas::has_drag_distance() const
{
    return std::abs(end_point_.x - start_point_.x) > 1.0f ||
           std::abs(end_point_.y - start_point_.y) > 1.0f;
}

void Canvas::trim_history()
{
    while (static_cast<int>(undo_history_.size()) > max_undo_steps_)
    {
        undo_history_.erase(undo_history_.begin());
    }

    while (static_cast<int>(redo_history_.size()) > max_undo_steps_)
    {
        redo_history_.erase(redo_history_.begin());
    }
}
}  // namespace USTC_CG