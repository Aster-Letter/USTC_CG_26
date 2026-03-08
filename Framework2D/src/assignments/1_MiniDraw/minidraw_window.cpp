#include "minidraw_window.h"

namespace
{
constexpr float kToolbarWidth = 180.0f;
constexpr float kPropertiesWidth = 300.0f;

void draw_config_editor(
    USTC_CG::Canvas* canvas,
    USTC_CG::Shape::Config& config,
    bool supports_fill,
    bool apply_to_selection)
{
    const bool line_color_changed =
        ImGui::ColorEdit4("Line Color", config.line_color);
    if (apply_to_selection && ImGui::IsItemActivated())
    {
        canvas->begin_selected_shape_edit();
    }
    if (apply_to_selection && line_color_changed)
    {
        canvas->set_selected_shape_config(config);
    }
    if (apply_to_selection && ImGui::IsItemDeactivatedAfterEdit())
    {
        canvas->end_selected_shape_edit();
    }

    const bool thickness_changed =
        ImGui::SliderFloat("Line Width", &config.line_thickness, 1.0f, 12.0f);
    if (apply_to_selection && ImGui::IsItemActivated())
    {
        canvas->begin_selected_shape_edit();
    }
    if (apply_to_selection && thickness_changed)
    {
        canvas->set_selected_shape_config(config);
    }
    if (apply_to_selection && ImGui::IsItemDeactivatedAfterEdit())
    {
        canvas->end_selected_shape_edit();
    }

    if (!supports_fill)
    {
        return;
    }

    const bool fill_changed = ImGui::Checkbox("Fill Shape", &config.is_filled);
    if (apply_to_selection && fill_changed)
    {
        canvas->set_selected_shape_config(config);
    }

    if (config.is_filled)
    {
        const bool fill_color_changed =
            ImGui::ColorEdit4("Fill Color", config.fill_color);
        if (apply_to_selection && ImGui::IsItemActivated())
        {
            canvas->begin_selected_shape_edit();
        }
        if (apply_to_selection && fill_color_changed)
        {
            canvas->set_selected_shape_config(config);
        }
        if (apply_to_selection && ImGui::IsItemDeactivatedAfterEdit())
        {
            canvas->end_selected_shape_edit();
        }
    }
}
}

namespace USTC_CG
{
MiniDraw::MiniDraw(const std::string& window_name) : Window(window_name)
{
    p_canvas_ = std::make_shared<Canvas>("Widget.Canvas");
}

MiniDraw::~MiniDraw()
{
}

void MiniDraw::draw()
{
    handle_shortcuts();
    draw_main_menu();

    if (flag_show_canvas_view_)
    {
        draw_toolbar();
        draw_properties_panel();
        draw_canvas();
    }

    draw_settings_window();
}

void MiniDraw::handle_shortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
    {
        p_canvas_->delete_selected_shape();
        return;
    }

    const bool command_down = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    if (!command_down)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        if (io.KeyShift)
        {
            p_canvas_->redo();
        }
        else
        {
            p_canvas_->undo();
        }
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        p_canvas_->redo();
        return;
    }
}

void MiniDraw::draw_main_menu()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, p_canvas_->can_undo()))
        {
            p_canvas_->undo();
        }

        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, p_canvas_->can_redo()))
        {
            p_canvas_->redo();
        }

        if (
            ImGui::MenuItem(
                "Delete Selected",
                "Del",
                false,
                p_canvas_->has_selected_shape()))
        {
            p_canvas_->delete_selected_shape();
        }

        ImGui::Separator();
        if (
            ImGui::MenuItem(
                "Clear Canvas", nullptr, false, p_canvas_->has_shapes()))
        {
            p_canvas_->clear_shape_list();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Show Workspace", nullptr, &flag_show_canvas_view_);
        if (ImGui::MenuItem("Settings"))
        {
            flag_show_settings_ = true;
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void MiniDraw::draw_toolbar()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float top_bar_height = ImGui::GetFrameHeight();
    const ImVec2 work_pos = viewport->WorkPos;
    const ImVec2 work_size = viewport->WorkSize;

    ImGui::SetNextWindowPos(ImVec2(work_pos.x, work_pos.y + top_bar_height));
    ImGui::SetNextWindowSize(
        ImVec2(kToolbarWidth, work_size.y - top_bar_height));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Tools", nullptr, flags))
    {
        ImGui::TextUnformatted("Tool Palette");
        ImGui::Separator();

        const struct
        {
            const char* label;
            Canvas::ShapeType type;
        } tools[] = {
            { "Select", Canvas::kDefault },
            { "Line", Canvas::kLine },
            { "Rect", Canvas::kRect },
            { "Ellipse", Canvas::kEllipse },
            { "Polygon", Canvas::kPolygon },
            { "Freehand", Canvas::kFreehand },
        };

        for (const auto& tool : tools)
        {
            if (ImGui::Selectable(tool.label, p_canvas_->shape_type() == tool.type))
            {
                if (tool.type == Canvas::kDefault)
                {
                    p_canvas_->set_default();
                }
                else if (p_canvas_->shape_type() == tool.type)
                {
                    p_canvas_->set_default();
                }
                else
                {
                    switch (tool.type)
                    {
                        case Canvas::kLine: p_canvas_->set_line(); break;
                        case Canvas::kRect: p_canvas_->set_rect(); break;
                        case Canvas::kEllipse: p_canvas_->set_ellipse(); break;
                        case Canvas::kPolygon: p_canvas_->set_polygon(); break;
                        case Canvas::kFreehand: p_canvas_->set_freehand(); break;
                        default: p_canvas_->set_default(); break;
                    }
                }
            }
        }

        ImGui::Spacing();
        if (p_canvas_->shape_type() == Canvas::kDefault)
        {
            ImGui::TextWrapped(
                "Select mode: click a shape to select it, drag to move it, then edit its style or transform in the Properties panel.");
        }
        else
        {
            ImGui::TextWrapped(
                "Drag to draw Line, Rect, Ellipse and Freehand. Polygon uses left click to add vertices and right click to finish.");
        }
    }
    ImGui::End();
}

void MiniDraw::draw_properties_panel()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float top_bar_height = ImGui::GetFrameHeight();
    const ImVec2 work_pos = viewport->WorkPos;
    const ImVec2 work_size = viewport->WorkSize;

    ImGui::SetNextWindowPos(ImVec2(
        work_pos.x + work_size.x - kPropertiesWidth,
        work_pos.y + top_bar_height));
    ImGui::SetNextWindowSize(
        ImVec2(kPropertiesWidth, work_size.y - top_bar_height));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Properties", nullptr, flags))
    {
        const Canvas::ShapeType active_tool = p_canvas_->shape_type();
        if (active_tool == Canvas::kDefault)
        {
            ImGui::TextUnformatted("Selection");
            ImGui::Separator();
            if (p_canvas_->has_selected_shape())
            {
                Shape::Config config = p_canvas_->selected_shape_config();
                ImGui::Text("Shape: %s", p_canvas_->selected_shape_type_name());
                draw_config_editor(
                    p_canvas_.get(),
                    config,
                    p_canvas_->selected_shape_supports_fill(),
                    true);

                float rotation_degrees =
                    p_canvas_->selected_shape_rotation_degrees();
                const bool rotation_changed = ImGui::SliderFloat(
                    "Rotation", &rotation_degrees, -180.0f, 180.0f, "%.1f deg");
                if (ImGui::IsItemActivated())
                {
                    p_canvas_->begin_selected_shape_edit();
                }
                if (rotation_changed)
                {
                    p_canvas_->set_selected_shape_rotation_degrees(
                        rotation_degrees);
                }
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    p_canvas_->end_selected_shape_edit();
                }

                ImGui::Spacing();
                ImGui::TextUnformatted("Transform");
                ImGui::Separator();
                if (ImGui::Button("Scale -10%", ImVec2(-1.0f, 0.0f)))
                {
                    p_canvas_->scale_selected_shape(0.9f);
                }
                if (ImGui::Button("Scale +10%", ImVec2(-1.0f, 0.0f)))
                {
                    p_canvas_->scale_selected_shape(1.1f);
                }
                if (ImGui::Button("Delete Selected", ImVec2(-1.0f, 0.0f)))
                {
                    p_canvas_->delete_selected_shape();
                }
            }
            else
            {
                ImGui::TextWrapped(
                    "No shape is selected. Use the Select tool, then click a shape on the canvas.");
            }
        }
        else
        {
            auto& config = p_canvas_->mutable_pending_config();
            ImGui::TextUnformatted("Next Shape Style");
            ImGui::Separator();
            draw_config_editor(
                p_canvas_.get(),
                config,
                p_canvas_->tool_supports_fill(active_tool),
                false);

            if (!p_canvas_->tool_supports_fill(active_tool))
            {
                config.is_filled = false;
            }
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("History");
        ImGui::Separator();
        if (ImGui::Button("Undo", ImVec2(-1.0f, 0.0f)))
        {
            p_canvas_->undo();
        }
        if (ImGui::Button("Redo", ImVec2(-1.0f, 0.0f)))
        {
            p_canvas_->redo();
        }
    }
    ImGui::End();
}

void MiniDraw::draw_canvas()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float top_bar_height = ImGui::GetFrameHeight();
    const ImVec2 work_pos = viewport->WorkPos;
    const ImVec2 work_size = viewport->WorkSize;
    const ImVec2 canvas_pos(work_pos.x + kToolbarWidth, work_pos.y + top_bar_height);
    const ImVec2 workspace_size(
        work_size.x - kToolbarWidth - kPropertiesWidth,
        work_size.y - top_bar_height);

    ImGui::SetNextWindowPos(canvas_pos);
    ImGui::SetNextWindowSize(workspace_size);
    if (ImGui::Begin(
            "Canvas Workspace",
            nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse))
    {
        ImGui::TextUnformatted("Canvas");
        ImGui::Separator();
        const auto& canvas_min = ImGui::GetCursorScreenPos();
        const auto& available_size = ImGui::GetContentRegionAvail();
        p_canvas_->set_attributes(canvas_min, available_size);
        p_canvas_->draw();
    }
    ImGui::End();
}

void MiniDraw::draw_settings_window()
{
    if (!flag_show_settings_)
    {
        return;
    }

    if (ImGui::Begin("Settings", &flag_show_settings_))
    {
        int max_undo = p_canvas_->max_undo_steps();
        if (ImGui::SliderInt("Max Undo Steps", &max_undo, 1, 200))
        {
            p_canvas_->set_max_undo_steps(max_undo);
        }

        float background_color[4] = {
            p_canvas_->background_color()[0],
            p_canvas_->background_color()[1],
            p_canvas_->background_color()[2],
            p_canvas_->background_color()[3],
        };
        if (ImGui::ColorEdit4("Canvas Background", background_color))
        {
            p_canvas_->set_background_color(background_color);
        }

        bool antialiasing = p_canvas_->antialiasing();
        if (ImGui::Checkbox("Enable Anti-Aliasing", &antialiasing))
        {
            p_canvas_->set_antialiasing(antialiasing);
        }
    }
    ImGui::End();
}
}  // namespace USTC_CG