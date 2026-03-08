#include "canvas_widget.h"

#include <algorithm>
#include <cmath>

#include "imgui.h"
#include "shapes/ellipse.h"
#include "shapes/freehand.h"
#include "shapes/line.h"
#include "shapes/polygon.h"
#include "shapes/rect.h"

namespace USTC_CG
{
namespace
{
constexpr float kDragStartThresholdSquared = 9.0f;
constexpr float kRadiansToDegrees = 57.29577951308232f;
constexpr float kDegreesToRadians = 0.017453292519943295f;
constexpr float kSelectionOutlinePadding = 6.0f;
}  // namespace

void Canvas::draw()
{
    validate_selection();
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

    if (shape_type_ == kDefault)
    {
        if (selection_drag_candidate_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            mouse_release_event();
        }
    }
    else if (
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

bool Canvas::tool_supports_fill(ShapeType type) const
{
    return type == kRect || type == kEllipse || type == kPolygon;
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

bool Canvas::has_selected_shape() const
{
    return selected_shape_internal() != nullptr;
}

const Shape* Canvas::selected_shape() const
{
    return selected_shape_internal();
}

Shape::Config Canvas::selected_shape_config() const
{
    const Shape* shape = selected_shape_internal();
    return shape ? shape->config() : Shape::Config {};
}

bool Canvas::selected_shape_supports_fill() const
{
    const Shape* shape = selected_shape_internal();
    return shape && shape->supports_fill();
}

const char* Canvas::selected_shape_type_name() const
{
    const Shape* shape = selected_shape_internal();
    return shape ? shape->type_name() : "None";
}

float Canvas::selected_shape_rotation_degrees() const
{
    const Shape* shape = selected_shape_internal();
    return shape ? shape->rotation_radians() * kRadiansToDegrees : 0.0f;
}

void Canvas::begin_selected_shape_edit()
{
    begin_edit_transaction();
}

void Canvas::end_selected_shape_edit()
{
    end_edit_transaction_internal();
}

void Canvas::set_selected_shape_config(const Shape::Config& config)
{
    Shape* shape = mutable_selected_shape();
    if (!shape)
    {
        return;
    }

    const bool started_here = !edit_transaction_active_;
    if (started_here)
    {
        begin_edit_transaction();
    }

    shape->mutable_shape_config() = config;

    if (started_here)
    {
        end_edit_transaction_internal();
    }
}

void Canvas::set_selected_shape_rotation_degrees(float degrees)
{
    Shape* shape = mutable_selected_shape();
    if (!shape)
    {
        return;
    }

    const bool started_here = !edit_transaction_active_;
    if (started_here)
    {
        begin_edit_transaction();
    }

    shape->set_rotation_radians(degrees * kDegreesToRadians);

    if (started_here)
    {
        end_edit_transaction_internal();
    }
}

void Canvas::scale_selected_shape(float factor)
{
    Shape* shape = mutable_selected_shape();
    if (!shape || factor <= 0.0f)
    {
        return;
    }

    const bool started_here = !edit_transaction_active_;
    if (started_here)
    {
        begin_edit_transaction();
    }

    shape->scale(factor, factor, shape->center());

    if (started_here)
    {
        end_edit_transaction_internal();
    }
}

void Canvas::delete_selected_shape()
{
    validate_selection();
    if (selected_shape_index_ < 0)
    {
        return;
    }

    save_undo_state();
    shape_list_.erase(shape_list_.begin() + selected_shape_index_);
    clear_selection();
}

void Canvas::clear_selection()
{
    selected_shape_index_ = -1;
    selection_drag_candidate_ = false;
    selection_drag_active_ = false;
    end_edit_transaction_internal();
}

void Canvas::set_default()
{
    cancel_current_shape();
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    cancel_current_shape();
    clear_selection();
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    cancel_current_shape();
    clear_selection();
    shape_type_ = kRect;
}

void Canvas::set_ellipse()
{
    cancel_current_shape();
    clear_selection();
    shape_type_ = kEllipse;
}

void Canvas::set_polygon()
{
    cancel_current_shape();
    clear_selection();
    shape_type_ = kPolygon;
}

void Canvas::set_freehand()
{
    cancel_current_shape();
    clear_selection();
    shape_type_ = kFreehand;
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
    clear_selection();
    shape_list_.clear();
}

void Canvas::undo()
{
    cancel_current_shape();
    clear_selection();
    if (undo_history_.empty())
    {
        return;
    }

    redo_history_.push_back(clone_shape_list(shape_list_));
    shape_list_ = clone_shape_list(undo_history_.back());
    undo_history_.pop_back();
}

void Canvas::redo()
{
    cancel_current_shape();
    clear_selection();
    if (redo_history_.empty())
    {
        return;
    }

    undo_history_.push_back(clone_shape_list(shape_list_));
    shape_list_ = clone_shape_list(redo_history_.back());
    redo_history_.pop_back();
    trim_history();
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
        draw_list->AddRectFilled(
            canvas_min_,
            canvas_max_,
            ImGui::ColorConvertFloat4ToU32(ImVec4(
                background_color_[0],
                background_color_[1],
                background_color_[2],
                background_color_[3])));
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }

    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    Shape::Config screen_config;
    screen_config.bias[0] = canvas_min_.x;
    screen_config.bias[1] = canvas_min_.y;
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

    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(screen_config);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(screen_config);
    }
    if (has_selected_shape())
    {
        draw_selection_overlay();
    }
    draw_list->PopClipRect();
    draw_list->Flags = previous_flags;
}

void Canvas::draw_selection_overlay()
{
    const Shape* shape = selected_shape_internal();
    if (!shape)
    {
        return;
    }

    const ImVec2 min_point = shape->bounds_min();
    const ImVec2 max_point = shape->bounds_max();
    const ImVec2 screen_min(
        canvas_min_.x + min_point.x - kSelectionOutlinePadding,
        canvas_min_.y + min_point.y - kSelectionOutlinePadding);
    const ImVec2 screen_max(
        canvas_min_.x + max_point.x + kSelectionOutlinePadding,
        canvas_min_.y + max_point.y + kSelectionOutlinePadding);
    const ImVec2 screen_center(
        canvas_min_.x + shape->center().x,
        canvas_min_.y + shape->center().y);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(
        screen_min, screen_max, IM_COL32(255, 215, 0, 255), 0.0f, 0, 1.5f);
    draw_list->AddCircleFilled(screen_center, 4.0f, IM_COL32(255, 215, 0, 255));
}

void Canvas::mouse_click_event()
{
    start_point_ = end_point_ = mouse_pos_in_canvas();

    if (shape_type_ == kDefault)
    {
        const int hit_index = find_shape_at(start_point_);
        if (hit_index >= 0)
        {
            selected_shape_index_ = hit_index;
            selection_drag_candidate_ = true;
            selection_drag_active_ = false;
            selection_press_pos_ = start_point_;
            selection_last_mouse_pos_ = start_point_;
        }
        else
        {
            clear_selection();
        }
        return;
    }

    if (shape_type_ == kPolygon)
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
        case kLine:
            current_shape_ = std::make_shared<Line>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        case kRect:
            current_shape_ = std::make_shared<Rect>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        case kEllipse:
            current_shape_ = std::make_shared<Ellipse>(
                start_point_.x,
                start_point_.y,
                end_point_.x,
                end_point_.y,
                pending_config_);
            break;
        case kFreehand:
            current_shape_ = std::make_shared<Freehand>(
                start_point_.x, start_point_.y, pending_config_);
            break;
        default:
            draw_status_ = false;
            break;
    }
}

void Canvas::mouse_move_event()
{
    if (shape_type_ == kDefault)
    {
        if (!selection_drag_candidate_ || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            return;
        }

        Shape* shape = mutable_selected_shape();
        if (!shape)
        {
            return;
        }

        end_point_ = mouse_pos_in_canvas();
        const float total_dx = end_point_.x - selection_press_pos_.x;
        const float total_dy = end_point_.y - selection_press_pos_.y;
        if (
            !selection_drag_active_ &&
            total_dx * total_dx + total_dy * total_dy >=
                kDragStartThresholdSquared)
        {
            begin_edit_transaction();
            selection_drag_active_ = true;
        }

        if (selection_drag_active_)
        {
            const float dx = end_point_.x - selection_last_mouse_pos_.x;
            const float dy = end_point_.y - selection_last_mouse_pos_.y;
            if (std::abs(dx) > 0.0f || std::abs(dy) > 0.0f)
            {
                shape->translate(dx, dy);
            }
            selection_last_mouse_pos_ = end_point_;
        }
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
    if (shape_type_ == kDefault)
    {
        if (selection_drag_active_)
        {
            end_edit_transaction_internal();
        }
        selection_drag_candidate_ = false;
        selection_drag_active_ = false;
        return;
    }

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
        should_store_shape = has_drag_distance();
    }
    else
    {
        should_store_shape = has_drag_distance();
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
    return ImVec2(io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
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

void Canvas::save_undo_state()
{
    undo_history_.push_back(clone_shape_list(shape_list_));
    redo_history_.clear();
    trim_history();
}

std::vector<std::shared_ptr<Shape>> Canvas::clone_shape_list(
    const std::vector<std::shared_ptr<Shape>>& shapes) const
{
    std::vector<std::shared_ptr<Shape>> result;
    result.reserve(shapes.size());
    for (const auto& shape : shapes)
    {
        result.push_back(shape ? shape->clone() : nullptr);
    }
    return result;
}

void Canvas::begin_edit_transaction()
{
    if (edit_transaction_active_ || !mutable_selected_shape())
    {
        return;
    }

    save_undo_state();
    edit_transaction_active_ = true;
}

void Canvas::end_edit_transaction_internal()
{
    edit_transaction_active_ = false;
}

int Canvas::find_shape_at(const ImVec2& position) const
{
    for (int i = static_cast<int>(shape_list_.size()) - 1; i >= 0; --i)
    {
        const auto& shape = shape_list_[i];
        if (shape && shape->hit_test(position.x, position.y, selection_tolerance_))
        {
            return i;
        }
    }
    return -1;
}

Shape* Canvas::mutable_selected_shape()
{
    validate_selection();
    if (selected_shape_index_ < 0)
    {
        return nullptr;
    }
    return shape_list_[selected_shape_index_].get();
}

const Shape* Canvas::selected_shape_internal() const
{
    if (
        selected_shape_index_ < 0 ||
        selected_shape_index_ >= static_cast<int>(shape_list_.size()))
    {
        return nullptr;
    }
    return shape_list_[selected_shape_index_].get();
}

void Canvas::validate_selection()
{
    if (
        selected_shape_index_ < 0 ||
        selected_shape_index_ >= static_cast<int>(shape_list_.size()))
    {
        selected_shape_index_ = -1;
    }
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