#pragma once

#include <imgui.h>

#include <memory>
#include <vector>

#include "shapes/shape.h"
#include "common/widget.h"

namespace USTC_CG
{

// Canvas class for drawing shapes.
class Canvas : public Widget
{
   public:
    // Inherits constructor from Component.
    using Widget::Widget;

    // Override the draw method from the parent Component class.
    void draw() override;

    // Enumeration for supported shape types.
    enum ShapeType
    {
        kDefault = 0,
        kLine = 1,
        kRect = 2,
        kEllipse = 3,
        kPolygon = 4,
        kFreehand = 5,
    };

    // Shape type setters.
    void set_default();
    void set_line();
    void set_rect();
    void set_ellipse();
    void set_polygon();
    void set_freehand();
    ShapeType shape_type() const;

    Shape::Config& mutable_pending_config();
    const Shape::Config& pending_config() const;
    bool tool_supports_fill(ShapeType type) const;

    // Clears all shapes from the canvas.
    void clear_shape_list();
    void undo();
    void redo();
    bool can_undo() const;
    bool can_redo() const;
    bool has_shapes() const;

    // Set canvas attributes (position and size).
    void set_attributes(const ImVec2& min, const ImVec2& size);

    // Controls the visibility of the canvas background.
    void show_background(bool flag);
    void set_background_color(const float color[4]);
    const float* background_color() const;
    void set_antialiasing(bool enabled);
    bool antialiasing() const;
    void set_max_undo_steps(int steps);
    int max_undo_steps() const;
    ImVec2 canvas_min() const;
    ImVec2 canvas_size() const;
    bool is_canvas_hovered() const;
    bool has_selected_shape() const;
    const Shape* selected_shape() const;
    Shape::Config selected_shape_config() const;
    bool selected_shape_supports_fill() const;
    const char* selected_shape_type_name() const;
    float selected_shape_rotation_degrees() const;
    void begin_selected_shape_edit();
    void end_selected_shape_edit();
    void set_selected_shape_config(const Shape::Config& config);
    void set_selected_shape_rotation_degrees(float degrees);
    void scale_selected_shape(float factor);
    void delete_selected_shape();
    void clear_selection();

   private:
    // Drawing functions.
    void draw_background();
    void draw_shapes();
    void draw_selection_overlay();
    void commit_shape(const std::shared_ptr<Shape>& shape);
    void cancel_current_shape();
    void save_undo_state();
    std::vector<std::shared_ptr<Shape>> clone_shape_list(
        const std::vector<std::shared_ptr<Shape>>& shapes) const;
    void begin_edit_transaction();
    void end_edit_transaction_internal();
    int find_shape_at(const ImVec2& position) const;
    Shape* mutable_selected_shape();
    const Shape* selected_shape_internal() const;
    void validate_selection();
    bool is_drag_shape(ShapeType type) const;
    bool has_drag_distance() const;
    void trim_history();

    // Event handlers for mouse interactions.
    void mouse_click_event();
    void mouse_move_event();
    void mouse_release_event();

    // Calculates mouse's relative position in the canvas.
    ImVec2 mouse_pos_in_canvas() const;

    // Canvas attributes.
    ImVec2 canvas_min_;         // Top-left corner of the canvas.
    ImVec2 canvas_max_;         // Bottom-right corner of the canvas.
    ImVec2 canvas_size_;        // Size of the canvas.
    bool draw_status_ = false;  // Is the canvas currently being drawn on.

    ImVec2 canvas_minimal_size_ = ImVec2(50.f, 50.f);
    float background_color_[4] = { 50.f / 255.f, 50.f / 255.f, 50.f / 255.f, 1.f };
    ImU32 border_color_ = IM_COL32(255, 255, 255, 255);
    bool show_background_ = true;  // Controls background visibility.
    bool antialiasing_ = true;
    int max_undo_steps_ = 50;

    // Mouse interaction status.
    bool is_hovered_ = false;
    bool is_active_ = false;

    // Current shape being drawn.
    ShapeType shape_type_ = kDefault;
    ImVec2 start_point_ = ImVec2(0.0f, 0.0f);
    ImVec2 end_point_ = ImVec2(0.0f, 0.0f);
    std::shared_ptr<Shape> current_shape_;
    Shape::Config pending_config_;

    // List of shapes drawn on the canvas.
    std::vector<std::shared_ptr<Shape>> shape_list_;
    std::vector<std::vector<std::shared_ptr<Shape>>> undo_history_;
    std::vector<std::vector<std::shared_ptr<Shape>>> redo_history_;

    int selected_shape_index_ = -1;
    bool selection_drag_candidate_ = false;
    bool selection_drag_active_ = false;
    bool edit_transaction_active_ = false;
    ImVec2 selection_press_pos_ = ImVec2(0.0f, 0.0f);
    ImVec2 selection_last_mouse_pos_ = ImVec2(0.0f, 0.0f);
    float selection_tolerance_ = 8.0f;
};

}  // namespace USTC_CG
