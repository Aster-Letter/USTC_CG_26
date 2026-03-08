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
        kEraser = 6,
    };

    // Shape type setters.
    void set_default();
    void set_line();
    void set_rect();
    void set_ellipse();
    void set_polygon();
    void set_freehand();
    void set_eraser();
    ShapeType shape_type() const;

    Shape::Config& mutable_pending_config();
    const Shape::Config& pending_config() const;

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
    void set_eraser_tolerance(float tolerance);
    float eraser_tolerance() const;
    ImVec2 canvas_min() const;
    ImVec2 canvas_size() const;
    bool is_canvas_hovered() const;

   private:
    // Drawing functions.
    void draw_background();
    void draw_shapes();
    void commit_shape(const std::shared_ptr<Shape>& shape);
    void cancel_current_shape();
    bool erase_shape_at(float x, float y);
    void save_undo_state();
    void draw_eraser_overlay();
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
    float eraser_tolerance_ = 10.0f;
};

}  // namespace USTC_CG
