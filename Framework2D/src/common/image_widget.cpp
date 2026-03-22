#include "common/image_widget.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "iostream"
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace USTC_CG
{
ImageWidget::ImageWidget(const std::string& label, const std::string& filename)
    : filename_(filename),
      Widget(label)
{
    glGenTextures(1, &tex_id_);
    auto image_data =
        stbi_load(filename.c_str(), &image_width_, &image_height_, NULL, 4);
    if (image_data == nullptr)
    {
        std::cout << "Failed to load image from file " << filename << std::endl;
        data_ = std::make_shared<Image>(image_width_, image_height_, 4);
    }
    else
    {
        std::cout << "Successfully load image from file " << filename
                  << std::endl;
        std::unique_ptr<unsigned char[]> tmp(image_data);
        data_ = std::make_shared<Image>(
            image_width_, image_height_, 4, std::move(tmp));
    }
    load_gltexture();
}

ImageWidget::~ImageWidget()
{
    glDeleteTextures(1, &tex_id_);
}

void ImageWidget::draw()
{
    draw_image();
}

void ImageWidget::set_position(const ImVec2& pos)
{
    position_ = pos;
}

ImVec2 ImageWidget::get_image_size() const
{
    return ImVec2((float)image_width_, (float)image_height_);
}

ImVec2 ImageWidget::get_rendered_size() const
{
    return ImVec2(image_width_ * scale_, image_height_ * scale_);
}

ImVec2 ImageWidget::get_image_screen_position() const
{
    return ImVec2(position_.x + pan_offset_.x, position_.y + pan_offset_.y);
}

ImVec2 ImageWidget::screen_to_image(const ImVec2& screen_pos, bool clamp) const
{
    ImVec2 image_pos(
        (screen_pos.x - position_.x - pan_offset_.x) / scale_,
        (screen_pos.y - position_.y - pan_offset_.y) / scale_);
    if (clamp)
    {
        image_pos.x = std::clamp(image_pos.x, 0.0f, (float)image_width_);
        image_pos.y = std::clamp(image_pos.y, 0.0f, (float)image_height_);
    }
    return image_pos;
}

ImVec2 ImageWidget::image_to_screen(const ImVec2& image_pos) const
{
    return ImVec2(
        position_.x + pan_offset_.x + image_pos.x * scale_,
        position_.y + pan_offset_.y + image_pos.y * scale_);
}

float ImageWidget::get_scale() const
{
    return scale_;
}

void ImageWidget::reset_view()
{
    scale_ = 1.0f;
    pan_offset_ = ImVec2(0.0f, 0.0f);
    is_panning_ = false;
}

void ImageWidget::update()
{
    load_gltexture();
}

void ImageWidget::save_to_disk(const std::string& filename)
{
    if (data_)
    {
        stbi_write_png(
            filename.c_str(),
            data_->width(),
            data_->height(),
            data_->channels(),
            data_->data(),
            data_->width() * data_->channels());
    }
}

void ImageWidget::load_gltexture()
{
    glBindTexture(GL_TEXTURE_2D, tex_id_);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture (different type of channels)
    if (data_->channels() == 3)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            image_width_,
            image_height_,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            data_->data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); 
    }
    else if (data_->channels() == 4)
    {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            image_width_,
            image_height_,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data_->data());
    }
    else
    {
        throw std::runtime_error("Unsupported number of channels");
    }
}

void ImageWidget::draw_image()
{
    auto draw_list = ImGui::GetWindowDrawList();
    if (data_)
    {
        ImVec2 p_min = get_image_screen_position();
        ImVec2 size = get_rendered_size();
        ImVec2 p_max = ImVec2(p_min.x + size.x, p_min.y + size.y);
        ImVec2 clip_min = ImGui::GetWindowPos();
        ImVec2 clip_max = ImVec2(
            clip_min.x + ImGui::GetWindowSize().x,
            clip_min.y + ImGui::GetWindowSize().y);
        draw_list->PushClipRect(clip_min, clip_max, true);
        draw_list->AddImage((intptr_t)tex_id_, p_min, p_max);
        draw_list->PopClipRect();
    }
}

void ImageWidget::handle_view_navigation(bool hovered)
{
    ImGuiIO& io = ImGui::GetIO();

    if (hovered && io.MouseWheel != 0.0f)
    {
        ImVec2 image_pos_before_zoom = screen_to_image(io.MousePos, false);
        float zoom_factor = std::pow(1.1f, io.MouseWheel);
        scale_ = std::clamp(scale_ * zoom_factor, 0.1f, 16.0f);
        pan_offset_ = ImVec2(
            io.MousePos.x - position_.x - image_pos_before_zoom.x * scale_,
            io.MousePos.y - position_.y - image_pos_before_zoom.y * scale_);
    }

    bool wants_pan = ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
                     ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (!is_panning_ && hovered && wants_pan)
    {
        is_panning_ = true;
        last_pan_mouse_pos_ = io.MousePos;
    }
    if (is_panning_)
    {
        if (wants_pan)
        {
            ImVec2 delta(
                io.MousePos.x - last_pan_mouse_pos_.x,
                io.MousePos.y - last_pan_mouse_pos_.y);
            pan_offset_.x += delta.x;
            pan_offset_.y += delta.y;
            last_pan_mouse_pos_ = io.MousePos;
        }
        else
        {
            is_panning_ = false;
        }
    }
}
}  // namespace USTC_CG