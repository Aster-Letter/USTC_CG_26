#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include "common/window.h"
#include "canvas_widget.h"

namespace USTC_CG
{
class MiniDraw : public Window
{
   public:
    explicit MiniDraw(const std::string& window_name);
    ~MiniDraw();

    void draw() override;

    protected:
     void post_render() override;

   private:
     void request_export_canvas();
     void process_export_directory_dialog();
     bool export_canvas_to_png(const std::filesystem::path& file_path);
    void handle_shortcuts();
    void draw_main_menu();
    void draw_toolbar();
    void draw_properties_panel();
    void draw_canvas();
    void draw_settings_window();

    std::shared_ptr<Canvas> p_canvas_ = nullptr;

    bool flag_show_canvas_view_ = true;
    bool flag_show_settings_ = false;
    bool export_prepare_frame_ = false;
    bool pending_export_canvas_ = false;
    std::string export_status_message_;
    std::filesystem::path export_directory_ = std::filesystem::current_path();
    std::array<char, 128> export_file_name_ = { 'm', 'i', 'n', 'i', 'd', 'r', 'a', 'w', '_', 'e', 'x', 'p', 'o', 'r', 't', '.', 'p', 'n', 'g', '\0' };
    ImVec2 canvas_capture_min_ = ImVec2(0.0f, 0.0f);
    ImVec2 canvas_capture_size_ = ImVec2(0.0f, 0.0f);
};
}  // namespace USTC_CG