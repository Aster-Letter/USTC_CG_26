#pragma once

#include "common/image_widget.h"
#include "shapes/shape.h"

#include <cstdint>

namespace USTC_CG
{
class SourceImageWidget : public ImageWidget
{
   public:
    struct ClonePatch
    {
        std::shared_ptr<Image> image;
        std::shared_ptr<Image> mask;
        ImVec2 anchor = ImVec2(0.0f, 0.0f);

        bool valid() const
        {
            return image != nullptr && mask != nullptr;
        }
    };

    enum RegionType
    {
        kDefault = 0,
        kRect = 1,
        kEllipse = 2,
        kFreehand = 3
    };

    explicit SourceImageWidget(
        const std::string& label,
        const std::string& filename);
    virtual ~SourceImageWidget() noexcept = default;

    void draw() override;

    void enable_selecting(bool flag);
    void select_region();
    std::shared_ptr<Image> get_region_mask();
    std::shared_ptr<Image> get_data();
    ImVec2 get_position() const;

    ClonePatch build_clone_patch() const;

    float get_clone_scale() const;
    void set_region_type(RegionType type) { region_type_ = type; }

    std::uint64_t get_selection_version() const;

   private:
    void mouse_click_event();
    void mouse_move_event();
    void mouse_release_event();
    ImVec2 mouse_pos_in_canvas() const;
    void update_selected_region();

    RegionType region_type_ = kRect;
    std::unique_ptr<Shape> selected_shape_;
    std::shared_ptr<Image> selected_region_mask_;
    std::uint64_t selection_version_ = 0;

    ImVec2 start_, end_;
    bool flag_enable_selecting_region_ = false;
    bool draw_status_ = false;
};

}  // namespace USTC_CG