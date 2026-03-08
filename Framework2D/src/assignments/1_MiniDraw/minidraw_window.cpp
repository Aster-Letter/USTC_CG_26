#include "minidraw_window.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <ImGuiFileDialog.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
constexpr float kToolbarWidth = 180.0f;
constexpr float kPropertiesWidth = 300.0f;
constexpr char kExportDirDialogKey[] = "MiniDrawExportDirectory";

std::string path_to_utf8(const std::filesystem::path& path)
{
    const auto utf8_path = path.u8string();
    return std::string(utf8_path.begin(), utf8_path.end());
}

std::filesystem::path utf8_to_path(const std::string& utf8_path)
{
    const auto* begin = reinterpret_cast<const char8_t*>(utf8_path.data());
    return std::filesystem::path(std::u8string(begin, begin + utf8_path.size()));
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
    const bool capture_this_frame = export_prepare_frame_;

    handle_shortcuts();
    draw_main_menu();

    if (flag_show_canvas_view_)
    {
        draw_toolbar();
        draw_properties_panel();
        draw_canvas();
    }

    if (!capture_this_frame)
    {
        draw_settings_window();
        process_export_directory_dialog();
    }

    if (capture_this_frame)
    {
        pending_export_canvas_ = true;
        export_prepare_frame_ = false;
    }
}

void MiniDraw::post_render()
{
    if (!pending_export_canvas_)
    {
        return;
    }

    pending_export_canvas_ = false;

    std::filesystem::path file_path = export_directory_;
    std::string file_name = export_file_name_.data();
    if (file_name.empty())
    {
        file_name = "minidraw_export.png";
    }
    if (std::filesystem::path(file_name).extension().empty())
    {
        file_name += ".png";
    }

    file_path /= file_name;
    if (export_canvas_to_png(file_path))
    {
        export_status_message_ = "Exported canvas to: " + path_to_utf8(file_path);
    }
    else if (export_status_message_.empty())
    {
        export_status_message_ = "Failed to export canvas.";
    }
}

void MiniDraw::request_export_canvas()
{
    export_prepare_frame_ = true;
    export_status_message_.clear();
}

void MiniDraw::process_export_directory_dialog()
{
    if (ImGuiFileDialog::Instance()->Display(kExportDirDialogKey))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            export_directory_ = utf8_to_path(
                ImGuiFileDialog::Instance()->GetCurrentPath());
            export_status_message_ = "Export folder updated.";
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

bool MiniDraw::export_canvas_to_png(const std::filesystem::path& file_path)
{
    if (canvas_capture_size_.x <= 1.0f || canvas_capture_size_.y <= 1.0f)
    {
        export_status_message_ = "Canvas region is empty, nothing to export.";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(file_path.parent_path(), ec);
    if (ec)
    {
        export_status_message_ = "Failed to create export folder.";
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    const float scale_x = io.DisplayFramebufferScale.x <= 0.0f ? 1.0f : io.DisplayFramebufferScale.x;
    const float scale_y = io.DisplayFramebufferScale.y <= 0.0f ? 1.0f : io.DisplayFramebufferScale.y;

    const int capture_x = std::max(0, static_cast<int>(std::lround(canvas_capture_min_.x * scale_x)));
    const int capture_width = std::max(1, static_cast<int>(std::lround(canvas_capture_size_.x * scale_x)));
    const int capture_height = std::max(1, static_cast<int>(std::lround(canvas_capture_size_.y * scale_y)));
    const int capture_y = std::max(
        0,
        height_ - static_cast<int>(std::lround((canvas_capture_min_.y + canvas_capture_size_.y) * scale_y)));

    std::vector<unsigned char> pixels(static_cast<size_t>(capture_width) * capture_height * 4U);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(
        capture_x,
        capture_y,
        capture_width,
        capture_height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data());

    stbi_flip_vertically_on_write(1);
    const int result = stbi_write_png(
        path_to_utf8(file_path).c_str(),
        capture_width,
        capture_height,
        4,
        pixels.data(),
        capture_width * 4);
    stbi_flip_vertically_on_write(0);

    if (result == 0)
    {
        export_status_message_ = "PNG writer failed.";
    }
    return result != 0;
}

void MiniDraw::handle_shortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
    {
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

    if (ImGui::IsKeyPressed(ImGuiKey_E, false) && flag_show_canvas_view_)
    {
        request_export_canvas();
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

        ImGui::Separator();
        if (
            ImGui::MenuItem(
                "Clear Canvas", nullptr, false, p_canvas_->has_shapes()))
        {
            p_canvas_->clear_shape_list();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Export Canvas", "Ctrl+E", false, flag_show_canvas_view_))
        {
            request_export_canvas();
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
            { "Line", Canvas::kLine },
            { "Rect", Canvas::kRect },
            { "Ellipse", Canvas::kEllipse },
            { "Polygon", Canvas::kPolygon },
            { "Freehand", Canvas::kFreehand },
            { "Eraser", Canvas::kEraser },
        };

        for (const auto& tool : tools)
        {
            if (ImGui::Selectable(tool.label, p_canvas_->shape_type() == tool.type))
            {
                if (p_canvas_->shape_type() == tool.type)
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
                        case Canvas::kEraser: p_canvas_->set_eraser(); break;
                        default: p_canvas_->set_default(); break;
                    }
                }
            }
        }

        ImGui::Spacing();
        ImGui::TextWrapped(
            "Drag to draw Line, Rect, Ellipse and Freehand. Polygon uses left click to add vertices and right click to finish. Eraser removes the topmost shape under the cursor.");

        if (p_canvas_->shape_type() == Canvas::kEraser)
        {
            ImGui::Spacing();
            float radius = p_canvas_->eraser_tolerance();
            if (ImGui::SliderFloat("Eraser Radius", &radius, 4.0f, 48.0f, "%.1f px"))
            {
                p_canvas_->set_eraser_tolerance(radius);
            }
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
        auto& config = p_canvas_->mutable_pending_config();

        ImGui::TextUnformatted("Next Shape Style");
        ImGui::Separator();
        ImGui::ColorEdit4("Line Color", config.line_color);
        ImGui::SliderFloat("Line Width", &config.line_thickness, 1.0f, 12.0f);
        ImGui::Checkbox("Fill Shape", &config.is_filled);
        if (config.is_filled)
        {
            ImGui::ColorEdit4("Fill Color", config.fill_color);
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

        if (p_canvas_->shape_type() == Canvas::kEraser)
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Eraser");
            ImGui::Separator();
            float radius = p_canvas_->eraser_tolerance();
            if (ImGui::SliderFloat("Brush Radius", &radius, 4.0f, 48.0f, "%.1f px"))
            {
                p_canvas_->set_eraser_tolerance(radius);
            }
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
        canvas_capture_min_ = canvas_min;
        canvas_capture_size_ = available_size;
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

        ImGui::Separator();
        ImGui::TextUnformatted("Export");
        ImGui::InputText("File Name", export_file_name_.data(), export_file_name_.size());
        ImGui::TextWrapped("Folder: %s", path_to_utf8(export_directory_).c_str());
        if (ImGui::Button("Choose Export Folder"))
        {
            IGFD::FileDialogConfig config;
            config.path = path_to_utf8(export_directory_);
            config.countSelectionMax = 1;
            ImGuiFileDialog::Instance()->OpenDialog(
                kExportDirDialogKey,
                "Choose Export Folder",
                nullptr,
                config);
        }
        if (ImGui::Button("Export Canvas PNG", ImVec2(-1.0f, 0.0f)))
        {
            request_export_canvas();
        }

        if (!export_status_message_.empty())
        {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", export_status_message_.c_str());
        }
    }
    ImGui::End();
}
}  // namespace USTC_CG