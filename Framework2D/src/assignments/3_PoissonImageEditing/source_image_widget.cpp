#include "source_image_widget.h"

#include "shapes/rect.h"
#include "shapes/ellipse.h"
#include "shapes/freehand.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
struct SelectionBounds
{
    int min_x = 0;
    int min_y = 0;
    int max_x = -1;
    int max_y = -1;

    bool valid() const
    {
        return max_x >= min_x && max_y >= min_y;
    }
};

SelectionBounds compute_selection_bounds(const USTC_CG::Image& mask)
{
    SelectionBounds bounds;
    bounds.min_x = mask.width();
    bounds.min_y = mask.height();
    for (int y = 0; y < mask.height(); ++y)
    {
        for (int x = 0; x < mask.width(); ++x)
        {
            if (mask.get_pixel(x, y)[0] == 0)
                continue;
            bounds.min_x = std::min(bounds.min_x, x);
            bounds.min_y = std::min(bounds.min_y, y);
            bounds.max_x = std::max(bounds.max_x, x);
            bounds.max_y = std::max(bounds.max_y, y);
        }
    }
    return bounds;
}

std::vector<unsigned char> sample_bilinear(
    const USTC_CG::Image& image,
    float x,
    float y)
{
    x = std::clamp(x, 0.0f, static_cast<float>(image.width() - 1));
    y = std::clamp(y, 0.0f, static_cast<float>(image.height() - 1));

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, image.width() - 1);
    const int y1 = std::min(y0 + 1, image.height() - 1);
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    const auto p00 = image.get_pixel(x0, y0);
    const auto p10 = image.get_pixel(x1, y0);
    const auto p01 = image.get_pixel(x0, y1);
    const auto p11 = image.get_pixel(x1, y1);

    std::vector<unsigned char> out(image.channels(), 0);
    for (int channel = 0; channel < image.channels(); ++channel)
    {
        const float top = (1.0f - tx) * static_cast<float>(p00[channel]) +
                          tx * static_cast<float>(p10[channel]);
        const float bottom = (1.0f - tx) * static_cast<float>(p01[channel]) +
                             tx * static_cast<float>(p11[channel]);
        const float value = (1.0f - ty) * top + ty * bottom;
        out[channel] = static_cast<unsigned char>(std::clamp(
            static_cast<int>(std::lround(value)), 0, 255));
    }
    return out;
}

bool mask_selected(const USTC_CG::Image& mask, float x, float y)
{
    const int sample_x = std::clamp(
        static_cast<int>(std::lround(x)), 0, mask.width() - 1);
    const int sample_y = std::clamp(
        static_cast<int>(std::lround(y)), 0, mask.height() - 1);
    return mask.get_pixel(sample_x, sample_y)[0] > 0;
}

}

namespace USTC_CG
{
using uchar = unsigned char;

SourceImageWidget::SourceImageWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        selected_region_mask_ =
            std::make_shared<Image>(data_->width(), data_->height(), 1);
}

void SourceImageWidget::draw()
{
    ImageWidget::draw();
    if (flag_enable_selecting_region_)
        select_region();
}

void SourceImageWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_region_ = flag;
}

void SourceImageWidget::select_region()
{
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
        mouse_release_event();

    // Region shape visualization
    if (selected_shape_)
    {
        ImVec2 screen_pos = get_image_screen_position();
        Shape::Config s = { .bias = { screen_pos.x, screen_pos.y },
                            .scale = get_scale(),
                            .line_color = { 255, 0, 0, 255 },
                            .line_thickness = 2.0f };
        selected_shape_->draw(s);
    }  
}

std::shared_ptr<Image> SourceImageWidget::get_region_mask()
{
    return selected_region_mask_;
}

std::shared_ptr<Image> SourceImageWidget::get_data()
{
    return data_;
}

ImVec2 SourceImageWidget::get_position() const
{
    return start_;
}

SourceImageWidget::ClonePatch SourceImageWidget::build_clone_patch() const
{
    ClonePatch patch;
    if (!data_ || !selected_region_mask_)
        return patch;

    const float clone_scale = get_clone_scale();

    const SelectionBounds selected_bounds =
        compute_selection_bounds(*selected_region_mask_);
    if (!selected_bounds.valid())
        return patch;

    SelectionBounds bounds = selected_bounds;

    const int context_padding = std::max(
        1,
        static_cast<int>(std::ceil(1.0f / std::max(clone_scale, 1e-3f))));
    bounds.min_x = std::max(0, bounds.min_x - context_padding);
    bounds.min_y = std::max(0, bounds.min_y - context_padding);
    bounds.max_x = std::min(data_->width() - 1, bounds.max_x + context_padding);
    bounds.max_y = std::min(data_->height() - 1, bounds.max_y + context_padding);

    const int src_width = bounds.max_x - bounds.min_x + 1;
    const int src_height = bounds.max_y - bounds.min_y + 1;
    const int patch_width = std::max(
        1, static_cast<int>(std::lround(static_cast<float>(src_width) * clone_scale)));
    const int patch_height = std::max(
        1,
        static_cast<int>(std::lround(static_cast<float>(src_height) * clone_scale)));

    patch.image = std::make_shared<Image>(patch_width, patch_height, data_->channels());
    patch.mask = std::make_shared<Image>(patch_width, patch_height, 1);
    patch.anchor = ImVec2(
        (start_.x - static_cast<float>(bounds.min_x)) * clone_scale,
        (start_.y - static_cast<float>(bounds.min_y)) * clone_scale);

    for (int y = 0; y < patch_height; ++y)
    {
        for (int x = 0; x < patch_width; ++x)
        {
            const float src_x =
                static_cast<float>(bounds.min_x) + (static_cast<float>(x) + 0.5f) /
                    clone_scale - 0.5f;
            const float src_y =
                static_cast<float>(bounds.min_y) + (static_cast<float>(y) + 0.5f) /
                    clone_scale - 0.5f;

            auto sampled_pixel = sample_bilinear(*data_, src_x, src_y);
            unsigned char soft_alpha = sampled_pixel.size() >= 4 ? sampled_pixel[3] : 255;

            if (sampled_pixel.size() >= 4)
                sampled_pixel[3] = soft_alpha;
            patch.image->set_pixel(x, y, sampled_pixel);
            patch.mask->set_pixel(x, y, { 0 });

            if (!mask_selected(*selected_region_mask_, src_x, src_y))
                continue;

            const unsigned char solver_alpha_threshold = 8;
            if (soft_alpha < solver_alpha_threshold)
                continue;

            patch.mask->set_pixel(x, y, { 255 });
        }
    }

    return patch;
}

float SourceImageWidget::get_clone_scale() const
{
    return std::max(get_scale(), 0.1f);
}

std::uint64_t SourceImageWidget::get_selection_version() const
{
    return selection_version_;
}

void SourceImageWidget::mouse_click_event()
{
    if (!draw_status_)
    {
        draw_status_ = true;
        start_ = end_ = mouse_pos_in_canvas();
        switch (region_type_)
        {
            case kDefault: break;
            case kRect:
                selected_shape_ =
                    std::make_unique<Rect>(start_.x, start_.y, end_.x, end_.y);
                break;
            case kEllipse:
                selected_shape_ =
                    std::make_unique<Ellipse>(start_.x, start_.y, end_.x, end_.y);
                break;
            case kFreehand:
                selected_shape_ =
                    std::make_unique<Freehand>(start_.x, start_.y);
                break;
            default: break;
        }
    }
}

void SourceImageWidget::mouse_move_event()
{
    if (draw_status_)
    {
        end_ = mouse_pos_in_canvas();
        if (selected_shape_)
            selected_shape_->update(end_.x, end_.y);
    }
}

void SourceImageWidget::mouse_release_event()
{
    if (draw_status_ && selected_shape_)
    {
        draw_status_ = false;
        update_selected_region();
    }
}

ImVec2 SourceImageWidget::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    return screen_to_image(io.MousePos, true);
}

void SourceImageWidget::update_selected_region()
{
    if (selected_shape_ == nullptr)
        return;
    std::vector<std::pair<int, int>> interior_pixels =
        selected_shape_->get_interior_pixels();
    std::memset(
        selected_region_mask_->data(),
        0,
        static_cast<size_t>(selected_region_mask_->width()) *
            selected_region_mask_->height() *
            selected_region_mask_->channels());
    for (const auto& [x, y] : interior_pixels)
    {
        if (x < 0 || x >= selected_region_mask_->width() ||
            y < 0 || y >= selected_region_mask_->height())
            continue;
        selected_region_mask_->set_pixel(x, y, { 255 });
    }
    ++selection_version_;
}
}  // namespace USTC_CG