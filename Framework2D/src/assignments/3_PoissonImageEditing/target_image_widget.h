#pragma once

#include "source_image_widget.h"
#include "seamless_clone.h"
#include "common/image_widget.h"

#include <cstdint>

namespace USTC_CG
{
class TargetImageWidget : public ImageWidget
{
   public:
    enum CloneType
    {
        kDefault = 0,
        kPaste = 1,
        kSeamless = 2,
        kMixing = 3
    };

    explicit TargetImageWidget(
        const std::string& label,
        const std::string& filename);
    virtual ~TargetImageWidget() noexcept = default;

    void draw() override;
    void set_source(std::shared_ptr<SourceImageWidget> source);
    void set_realtime(bool flag);
    void restore();
    void set_paste();
    void set_seamless();
    void set_mixing();
    void clone();

   private:
    void mouse_click_event();
    void mouse_move_event();
    void mouse_release_event();
    void invalidate_cached_solver();
    ImVec2 mouse_pos_in_canvas() const;

    std::shared_ptr<Image> back_up_;
    std::shared_ptr<SourceImageWidget> source_image_;
    CloneType clone_type_ = kDefault;

    ImVec2 mouse_position_;
    bool edit_status_ = false;
    bool flag_realtime_updating = false;

    // Cached Poisson solver for realtime updates
    std::unique_ptr<SeamlessClone> solver_;
    std::uint64_t cached_selection_version_ = 0;
    CloneType cached_clone_type_ = kDefault;
    float cached_clone_scale_ = 1.0f;
};
}  // namespace USTC_CG