#pragma once

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

   private:
    void handle_shortcuts();
    void draw_main_menu();
    void draw_toolbar();
    void draw_properties_panel();
    void draw_canvas();
    void draw_settings_window();

    std::shared_ptr<Canvas> p_canvas_ = nullptr;

    bool flag_show_canvas_view_ = true;
    bool flag_show_settings_ = false;
};
}  // namespace USTC_CG