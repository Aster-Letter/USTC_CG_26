#include "warping_widget.h"

#include "warper/IDW_warper.h"
#include "warper/RBF_warper.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace
{
using uchar = unsigned char;
using Point = Eigen::Vector2d;

constexpr double kSamplingEpsilon = 1e-6;

std::vector<uchar> make_background_pixel(int channels, uchar gray)
{
    if (channels == 4)
    {
        return { gray, gray, gray, 255 };
    }

    return std::vector<uchar>(static_cast<std::size_t>(channels), gray);
}

void clear_image(USTC_CG::Image* image, uchar gray)
{
    const auto background = make_background_pixel(image->channels(), gray);
    for (int y = 0; y < image->height(); ++y)
    {
        for (int x = 0; x < image->width(); ++x)
        {
            image->set_pixel(x, y, background);
        }
    }
}

ImVec2 clamp_to_image(const ImVec2& point, int width, int height)
{
    return ImVec2(
        std::clamp(point.x, 0.0f, static_cast<float>(width - 1)),
        std::clamp(point.y, 0.0f, static_cast<float>(height - 1)));
}

bool get_pixel_bilinear(
    const USTC_CG::Image& image,
    double x,
    double y,
    std::vector<uchar>* color)
{
    if (x < 0.0 || y < 0.0 || x > image.width() - 1.0 || y > image.height() - 1.0)
    {
        return false;
    }

    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, image.width() - 1);
    const int y1 = std::min(y0 + 1, image.height() - 1);
    const double tx = x - x0;
    const double ty = y - y0;

    const auto c00 = image.get_pixel(x0, y0);
    const auto c10 = image.get_pixel(x1, y0);
    const auto c01 = image.get_pixel(x0, y1);
    const auto c11 = image.get_pixel(x1, y1);

    color->assign(static_cast<std::size_t>(image.channels()), 0);
    for (int channel = 0; channel < image.channels(); ++channel)
    {
        const double top = (1.0 - tx) * c00[channel] + tx * c10[channel];
        const double bottom = (1.0 - tx) * c01[channel] + tx * c11[channel];
        (*color)[static_cast<std::size_t>(channel)] = static_cast<uchar>(
            std::clamp(std::lround((1.0 - ty) * top + ty * bottom), 0l, 255l));
    }

    return true;
}

void append_control_point_pair(
    const ImVec2& start_point,
    const ImVec2& end_point,
    std::vector<Point>* source_points,
    std::vector<Point>* target_points)
{
    const Point source(end_point.x, end_point.y);
    const Point target(start_point.x, start_point.y);

    for (std::size_t index = 0; index < source_points->size(); ++index)
    {
        if (((*source_points)[index] - source).norm() < kSamplingEpsilon)
        {
            (*target_points)[index] = target;
            return;
        }
    }

    source_points->push_back(source);
    target_points->push_back(target);
}

template <typename TWarper, typename TConfigure>
bool apply_inverse_warp(
    const USTC_CG::Image& image,
    const std::vector<ImVec2>& start_points,
    const std::vector<ImVec2>& end_points,
    TConfigure configure,
    USTC_CG::Image* warped_image)
{
    std::vector<Point> source_points;
    std::vector<Point> target_points;
    source_points.reserve(start_points.size());
    target_points.reserve(end_points.size());

    for (std::size_t index = 0; index < start_points.size(); ++index)
    {
        append_control_point_pair(
            start_points[index], end_points[index], &source_points, &target_points);
    }

    if (source_points.empty())
    {
        return false;
    }

    TWarper warper;
    configure(&warper);
    warper.set_control_points(source_points, target_points);

    for (int y = 0; y < image.height(); ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            const Point source = warper.warp(Point(x, y));
            std::vector<uchar> pixel;
            if (get_pixel_bilinear(image, source.x(), source.y(), &pixel))
            {
                warped_image->set_pixel(x, y, pixel);
            }
        }
    }

    return true;
}
}  // namespace

namespace USTC_CG
{
WarpingWidget::WarpingWidget(const std::string& label, const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
    {
        back_up_ = std::make_shared<Image>(*data_);
    }
}

void WarpingWidget::draw()
{
    ImageWidget::draw();
    if (flag_enable_selecting_points_)
    {
        select_points();
    }
}

void WarpingWidget::invert()
{
    for (int x = 0; x < data_->width(); ++x)
    {
        for (int y = 0; y < data_->height(); ++y)
        {
            const auto color = data_->get_pixel(x, y);
            data_->set_pixel(
                x,
                y,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    update();
}

void WarpingWidget::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    const int width = data_->width();
    const int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int x = 0; x < width; ++x)
            {
                for (int y = 0; y < height; ++y)
                {
                    data_->set_pixel(
                        x,
                        y,
                        image_tmp.get_pixel(width - 1 - x, height - 1 - y));
                }
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                for (int y = 0; y < height; ++y)
                {
                    data_->set_pixel(x, y, image_tmp.get_pixel(width - 1 - x, y));
                }
            }
        }
    }
    else if (is_vertical)
    {
        for (int x = 0; x < width; ++x)
        {
            for (int y = 0; y < height; ++y)
            {
                data_->set_pixel(x, y, image_tmp.get_pixel(x, height - 1 - y));
            }
        }
    }

    update();
}

void WarpingWidget::gray_scale()
{
    for (int x = 0; x < data_->width(); ++x)
    {
        for (int y = 0; y < data_->height(); ++y)
        {
            const auto color = data_->get_pixel(x, y);
            const uchar gray_value = static_cast<uchar>((color[0] + color[1] + color[2]) / 3);
            data_->set_pixel(x, y, { gray_value, gray_value, gray_value });
        }
    }
    update();
}

void WarpingWidget::warping()
{
    if (!data_)
    {
        return;
    }

    Image warped_image(*data_);
    clear_image(
        &warped_image,
        static_cast<uchar>(std::clamp(std::lround(background_gray_), 0l, 255l)));

    switch (warping_type_)
    {
        case kDefault: break;
        case kFisheye:
        {
            for (int y = 0; y < data_->height(); ++y)
            {
                for (int x = 0; x < data_->width(); ++x)
                {
                    const auto [source_x, source_y] =
                        fisheye_inverse_warping(x, y, data_->width(), data_->height());
                    std::vector<uchar> pixel;
                    if (get_pixel_bilinear(*data_, source_x, source_y, &pixel))
                    {
                        warped_image.set_pixel(x, y, pixel);
                    }
                }
            }
            break;
        }
        case kIDW:
        {
            if (start_points_.size() != end_points_.size() || start_points_.empty())
            {
                std::cout << "IDW requires at least one valid control point pair."
                          << std::endl;
                return;
            }

            if (!apply_inverse_warp<IDWWarper>(
                    *data_,
                    start_points_,
                    end_points_,
                    [this](IDWWarper* warper)
                    {
                        warper->set_weight_power(idw_power_);
                    },
                    &warped_image))
            {
                std::cout << "IDW received degenerate control points." << std::endl;
                return;
            }
            break;
        }
        case kRBF:
        {
            if (start_points_.size() != end_points_.size() || start_points_.empty())
            {
                std::cout << "RBF requires at least one valid control point pair."
                          << std::endl;
                return;
            }

            if (!apply_inverse_warp<RBFWarper>(
                    *data_,
                    start_points_,
                    end_points_,
                    [this](RBFWarper* warper)
                    {
                        warper->set_radius_scale(rbf_radius_scale_);
                    },
                    &warped_image))
            {
                std::cout << "RBF received degenerate control points." << std::endl;
                return;
            }
            break;
        }
        default: break;
    }

    *data_ = std::move(warped_image);
    update();
}

void WarpingWidget::restore()
{
    *data_ = *back_up_;
    draw_status_ = false;
    init_selections();
    update();
}

void WarpingWidget::set_default()
{
    warping_type_ = kDefault;
}

void WarpingWidget::set_fisheye()
{
    warping_type_ = kFisheye;
}

void WarpingWidget::set_IDW()
{
    warping_type_ = kIDW;
}

void WarpingWidget::set_RBF()
{
    warping_type_ = kRBF;
}

void WarpingWidget::set_fisheye_strength(float strength)
{
    fisheye_strength_ = std::max(strength, 0.1f);
}

void WarpingWidget::set_idw_power(float power)
{
    idw_power_ = std::max(power, 1.01f);
}

void WarpingWidget::set_rbf_radius_scale(float scale)
{
    rbf_radius_scale_ = std::max(scale, 0.05f);
}

void WarpingWidget::reset_warping_parameters()
{
    fisheye_strength_ = 10.0f;
    idw_power_ = 2.0f;
    rbf_radius_scale_ = 1.0f;
    background_gray_ = 96.0f;
}

float WarpingWidget::fisheye_strength() const
{
    return fisheye_strength_;
}

float WarpingWidget::idw_power() const
{
    return idw_power_;
}

float WarpingWidget::rbf_radius_scale() const
{
    return rbf_radius_scale_;
}

void WarpingWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}

void WarpingWidget::select_points()
{
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(static_cast<float>(image_width_), static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);

    const bool is_hovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = clamp_to_image(
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y),
            image_width_,
            image_height_);
        end_ = start_;
    }

    if (draw_status_)
    {
        end_ = clamp_to_image(
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y),
            image_width_,
            image_height_);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            start_points_.push_back(start_);
            end_points_.push_back(end_);
            draw_status_ = false;
        }
    }

    auto draw_list = ImGui::GetWindowDrawList();
    for (std::size_t index = 0; index < start_points_.size(); ++index)
    {
        const ImVec2 s(
            start_points_[index].x + position_.x,
            start_points_[index].y + position_.y);
        const ImVec2 e(
            end_points_[index].x + position_.x,
            end_points_[index].y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }

    if (draw_status_)
    {
        const ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        const ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
}

void WarpingWidget::init_selections()
{
    start_points_.clear();
    end_points_.clear();
}

std::pair<double, double> WarpingWidget::fisheye_inverse_warping(
    double x,
    double y,
    int width,
    int height) const
{
    const double center_x = width / 2.0;
    const double center_y = height / 2.0;
    const double dx = x - center_x;
    const double dy = y - center_y;
    const double target_distance = std::sqrt(dx * dx + dy * dy);

    if (target_distance < kSamplingEpsilon)
    {
        return { center_x, center_y };
    }

    const double source_distance =
        std::pow(target_distance / std::max(static_cast<double>(fisheye_strength_), 0.1), 2.0);
    const double ratio = source_distance / target_distance;
    return { center_x + dx * ratio, center_y + dy * ratio };
}
}  // namespace USTC_CG