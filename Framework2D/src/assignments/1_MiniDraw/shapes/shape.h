#pragma once

#include <imgui.h>

#include <memory>

namespace USTC_CG
{
class Shape
{
   public:
    // Draw Settings
    struct Config
    {
        // Offset to convert canvas position to screen position
        float bias[2] = { 0.f, 0.f };
        // ImGui editors work natively with normalized RGBA floats.
        float line_color[4] = { 1.f, 0.f, 0.f, 1.f };
        float line_thickness = 2.0f;
        bool is_filled = false;
        float fill_color[4] = { 1.f, 1.f, 1.f, 1.f };
    };

   public:
    Shape() = default;
    explicit Shape(const Config& config) : config_(config) {}
    virtual ~Shape() = default;

    /**
     * Draws the shape on the screen.
     * This is a pure virtual function that must be implemented by all derived
     * classes.
     *
     * @param config The configuration settings for drawing, including line
     * color, thickness, and bias.
     *               - line_color defines the color of the shape's outline.
     *               - line_thickness determines how thick the outline will be.
     *               - bias is used to adjust the shape's position on the
     * screen.
     */
    virtual void draw(const Config& config) const = 0;
    /**
     * Updates the state of the shape.
     * This function allows for dynamic modification of the shape, in response
     * to user interactions like dragging.
     *
     * @param x, y Dragging point. e.g. end point of a line.
     */
    virtual void update(float x, float y) = 0;
    virtual std::shared_ptr<Shape> clone() const = 0;
    virtual bool hit_test(float x, float y, float tolerance) const = 0;
    virtual void translate(float dx, float dy) = 0;
    virtual void scale(float scale_x, float scale_y, const ImVec2& anchor) = 0;
    virtual void rotate(float radians, const ImVec2& anchor) = 0;
    virtual ImVec2 center() const = 0;
    virtual ImVec2 bounds_min() const = 0;
    virtual ImVec2 bounds_max() const = 0;
    virtual float rotation_radians() const = 0;
    virtual void set_rotation_radians(float radians) = 0;
    virtual const char* type_name() const = 0;
    /**
     * Adds a control point to the shape.
     * This function is used to add control points to the shape, which can be
     * used to modify the shape's appearance.
     *
     * @param x, y Control point to be added. e.g. vertex of a polygon.
     */
    virtual void add_control_point(float x, float y)
    {
        static_cast<void>(x);
        static_cast<void>(y);
    }

    virtual bool supports_fill() const
    {
        return false;
    }

    Config& mutable_shape_config()
    {
        return config_;
    }

    const Config& config() const
    {
        return config_;
    }

   protected:
    const Config& shape_config() const
    {
        return config_;
    }

    static ImU32 to_imcolor(const float color[4])
    {
        return ImGui::ColorConvertFloat4ToU32(
            ImVec4(color[0], color[1], color[2], color[3]));
    }

   private:
    Config config_;
};
}  // namespace USTC_CG