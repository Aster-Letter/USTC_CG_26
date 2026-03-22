#include "target_image_widget.h"

#include <cmath>

namespace USTC_CG
{
using uchar = unsigned char;

namespace
{
std::vector<unsigned char> alpha_blend_pixel(
    const std::vector<unsigned char>& src,
    const std::vector<unsigned char>& dst)
{
    if (src.empty())
        return dst;

    std::vector<unsigned char> out = dst;
    const float alpha = (src.size() >= 4) ? (static_cast<float>(src[3]) / 255.0f)
                                          : 1.0f;
    const int blend_channels = std::min<int>(3, std::min(src.size(), dst.size()));
    for (int c = 0; c < blend_channels; ++c)
    {
        const float blended =
            alpha * static_cast<float>(src[c]) +
            (1.0f - alpha) * static_cast<float>(dst[c]);
        out[c] = static_cast<unsigned char>(std::clamp(
            static_cast<int>(std::lround(blended)), 0, 255));
    }
    if (out.size() >= 4)
        out[3] = 255;
    return out;
}
}  // namespace

TargetImageWidget::TargetImageWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
}

void TargetImageWidget::draw()
{
    ImageWidget::draw();
    ImGui::SetCursorScreenPos(get_image_screen_position());
    ImGui::InvisibleButton(
        label_.c_str(),
        get_rendered_size(),
        ImGuiButtonFlags_MouseButtonLeft);
    bool is_hovered_ = ImGui::IsItemHovered();
    handle_view_navigation(is_hovered_);

    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        mouse_click_event();
    }
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        mouse_release_event();
    }
}

void TargetImageWidget::set_source(std::shared_ptr<SourceImageWidget> source)
{
    source_image_ = source;
    invalidate_cached_solver();
}

void TargetImageWidget::set_realtime(bool flag)
{
    flag_realtime_updating = flag;
}

void TargetImageWidget::restore()
{
    *data_ = *back_up_;
    update();
}

void TargetImageWidget::set_paste()
{
    clone_type_ = kPaste;
}

void TargetImageWidget::set_seamless()
{
    clone_type_ = kSeamless;
}

void TargetImageWidget::set_mixing()
{
    clone_type_ = kMixing;
}

void TargetImageWidget::clone()
{
    if (data_ == nullptr || source_image_ == nullptr ||
        source_image_->get_region_mask() == nullptr)
        return;

    SourceImageWidget::ClonePatch patch = source_image_->build_clone_patch();
    if (!patch.valid())
        return;

    std::shared_ptr<Image> mask = patch.mask;
    std::uint64_t selection_version = source_image_->get_selection_version();
    float clone_scale = source_image_->get_clone_scale();

    switch (clone_type_)
    {
        case USTC_CG::TargetImageWidget::kDefault: break;
        case USTC_CG::TargetImageWidget::kPaste:
        {
            restore();

            for (int x = 0; x < mask->width(); ++x)
            {
                for (int y = 0; y < mask->height(); ++y)
                {
                    int tar_x =
                        static_cast<int>(mouse_position_.x - patch.anchor.x) + x;
                    int tar_y =
                        static_cast<int>(mouse_position_.y - patch.anchor.y) + y;
                    if (0 <= tar_x && tar_x < image_width_ && 0 <= tar_y &&
                        tar_y < image_height_ && mask->get_pixel(x, y)[0] > 0)
                    {
                        auto src_pixel = patch.image->get_pixel(x, y);
                        auto dst_pixel = data_->get_pixel(tar_x, tar_y);
                        data_->set_pixel(
                            tar_x,
                            tar_y,
                            alpha_blend_pixel(src_pixel, dst_pixel));
                    }
                }
            }
            break;
        }
        case USTC_CG::TargetImageWidget::kSeamless:
        case USTC_CG::TargetImageWidget::kMixing:
        {
            restore();

            auto source_data = patch.image;
            auto selected_pos = patch.anchor;
            int off_x = static_cast<int>(mouse_position_.x) -
                        static_cast<int>(selected_pos.x);
            int off_y = static_cast<int>(mouse_position_.y) -
                        static_cast<int>(selected_pos.y);

            GradientMode gm = (clone_type_ == kMixing)
                                  ? GradientMode::kMixing
                                  : GradientMode::kImporting;

            bool need_rebuild = (!solver_ ||
                                 cached_selection_version_ != selection_version ||
                                 cached_clone_type_ != clone_type_ ||
                                 std::abs(cached_clone_scale_ - clone_scale) > 1e-4f);
            if (need_rebuild)
            {
                solver_ = std::make_unique<SeamlessClone>(
                    source_data,
                    back_up_,
                    mask,
                    off_x,
                    off_y,
                    gm);
                solver_->precompute();
                cached_selection_version_ = selection_version;
                cached_clone_type_ = clone_type_;
                cached_clone_scale_ = clone_scale;
            }

            auto result = solver_->solve_with_new_offset(off_x, off_y);
            *data_ = *result;
            break;
        }
        default: break;
    }

    update();
}

void TargetImageWidget::invalidate_cached_solver()
{
    solver_.reset();
    cached_selection_version_ = 0;
    cached_clone_type_ = kDefault;
    cached_clone_scale_ = 1.0f;
}

void TargetImageWidget::mouse_click_event()
{
    edit_status_ = true;
    mouse_position_ = mouse_pos_in_canvas();
    clone();
}

void TargetImageWidget::mouse_move_event()
{
    if (edit_status_)
    {
        mouse_position_ = mouse_pos_in_canvas();
        if (flag_realtime_updating || clone_type_ == kPaste)
            clone();
    }
}

void TargetImageWidget::mouse_release_event()
{
    if (edit_status_)
    {
        edit_status_ = false;
    }
}

ImVec2 TargetImageWidget::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    return screen_to_image(io.MousePos, true);
}
}  // namespace USTC_CG