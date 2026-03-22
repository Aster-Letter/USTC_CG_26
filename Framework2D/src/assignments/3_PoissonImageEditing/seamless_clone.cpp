#include "seamless_clone.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace USTC_CG
{
using uchar = unsigned char;

namespace
{
double pixel_value(const Image& img, int x, int y, int ch)
{
    if (x < 0 || x >= img.width() || y < 0 || y >= img.height())
        return 0.0;
    return static_cast<double>(img.get_pixel(x, y)[ch]);
}

double pixel_value_clamped(const Image& img, int x, int y, int ch)
{
    if (img.width() <= 0 || img.height() <= 0)
        return 0.0;
    x = std::clamp(x, 0, img.width() - 1);
    y = std::clamp(y, 0, img.height() - 1);
    return static_cast<double>(img.get_pixel(x, y)[ch]);
}
}  // namespace

SeamlessClone::SeamlessClone(
    std::shared_ptr<Image> source,
    std::shared_ptr<Image> target,
    std::shared_ptr<Image> mask,
    int offset_x,
    int offset_y,
        GradientMode mode)
    : src_img_(std::move(source)),
      tar_img_(std::move(target)),
      mask_(std::move(mask)),
      offset_x_(offset_x),
      offset_y_(offset_y),
            mode_(mode)
{
    // Compute tight bounding box of selected (nonzero) pixels in the mask.
    int min_x = mask_->width(), min_y = mask_->height();
    int max_x = -1, max_y = -1;
    for (int y = 0; y < mask_->height(); ++y)
        for (int x = 0; x < mask_->width(); ++x)
            if (mask_->get_pixel(x, y)[0] > 0)
            {
                min_x = std::min(min_x, x);
                min_y = std::min(min_y, y);
                max_x = std::max(max_x, x);
                max_y = std::max(max_y, y);
            }

    if (max_x < 0)
    {
        region_w_ = 0;
        region_h_ = 0;
        return;
    }
    src_min_x_ = min_x;
    src_min_y_ = min_y;
    region_w_ = max_x - min_x + 1;
    region_h_ = max_y - min_y + 1;

    index_map_.assign(static_cast<size_t>(region_w_) * region_h_, -1);
    active_pixels_.reserve(static_cast<size_t>(region_w_) * region_h_);
    for (int ly = 0; ly < region_h_; ++ly)
    {
        for (int lx = 0; lx < region_w_; ++lx)
        {
            if (inside_mask(lx, ly))
            {
                index_map_[bbox_idx(lx, ly)] = (int)active_pixels_.size();
                active_pixels_.emplace_back(lx, ly);
            }
        }
    }
}

bool SeamlessClone::inside_bbox(int lx, int ly) const
{
    return 0 <= lx && lx < region_w_ && 0 <= ly && ly < region_h_;
}

bool SeamlessClone::inside_mask(int lx, int ly) const
{
    if (!inside_bbox(lx, ly))
        return false;
    return mask_->get_pixel(src_x(lx), src_y(ly))[0] > 0;
}

void SeamlessClone::build_matrix()
{
    int n = (int)active_pixels_.size();
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(static_cast<size_t>(n) * 5);

    const int dx[] = { -1, 1, 0, 0 };
    const int dy[] = { 0, 0, -1, 1 };

    for (const auto& [lx, ly] : active_pixels_)
    {
        int i = idx(lx, ly);
        triplets.emplace_back(i, i, 4.0);
        for (int d = 0; d < 4; ++d)
        {
            int nx = lx + dx[d], ny = ly + dy[d];
            if (inside_mask(nx, ny))
                triplets.emplace_back(i, idx(nx, ny), -1.0);
        }
    }

    A_.resize(n, n);
    A_.setFromTriplets(triplets.begin(), triplets.end());
}

Eigen::VectorXd SeamlessClone::build_rhs(int c) const
{
    int n = (int)active_pixels_.size();
    Eigen::VectorXd B(n);
    B.setZero();

    const int dx[] = { -1, 1, 0, 0 };
    const int dy[] = { 0, 0, -1, 1 };

    for (const auto& [lx, ly] : active_pixels_)
    {
        int i = idx(lx, ly);
        int sx = src_x(lx), sy = src_y(ly);
        int tx = tar_x(lx), ty = tar_y(ly);

        for (int d = 0; d < 4; ++d)
        {
            int nx = lx + dx[d], ny = ly + dy[d];
            int sx_n = sx + dx[d], sy_n = sy + dy[d];
            int tx_n = tx + dx[d], ty_n = ty + dy[d];

            double grad_src = pixel_value_clamped(*src_img_, sx, sy, c) -
                              pixel_value_clamped(*src_img_, sx_n, sy_n, c);
            double guidance = grad_src;
            if (mode_ == GradientMode::kMixing)
            {
                double grad_tar = pixel_value(*tar_img_, tx, ty, c) -
                                  pixel_value(*tar_img_, tx_n, ty_n, c);
                if (std::abs(grad_tar) > std::abs(grad_src))
                    guidance = grad_tar;
            }
            B(i) += guidance;

            if (!inside_mask(nx, ny))
                B(i) += pixel_value(*tar_img_, tx_n, ty_n, c);
        }
    }
    return B;
}

// Pre-factorize.
void SeamlessClone::precompute()
{
    if (active_pixels_.empty())
        return;
    build_matrix();
    solver_.compute(A_);
    factorized_ = solver_.info() == Eigen::Success;
    if (!factorized_)
    {
        std::cerr << "[SeamlessClone] factorization failed, active_pixels="
                  << active_pixels_.size() << std::endl;
    }
}

// Solve and produce result image.
std::shared_ptr<Image> SeamlessClone::solve()
{
    auto result = std::make_shared<Image>(*tar_img_);
    if (active_pixels_.empty())
        return result;

    if (!factorized_)
        precompute();
    if (!factorized_)
        return result;

    int channels = std::min(src_img_->channels(), tar_img_->channels());
    channels = std::min(channels, 3);

    for (int c = 0; c < channels; ++c)
    {
        Eigen::VectorXd B = build_rhs(c);
        Eigen::VectorXd sol = solver_.solve(B);
        if (solver_.info() != Eigen::Success)
        {
            std::cerr << "[SeamlessClone] solve failed at channel " << c
                      << ", active_pixels=" << active_pixels_.size()
                      << std::endl;
            continue;
        }

        for (const auto& [lx, ly] : active_pixels_)
        {
            int tx = tar_x(lx), ty = tar_y(ly);
            if (tx < 0 || tx >= tar_img_->width() ||
                ty < 0 || ty >= tar_img_->height())
                continue;

            double val = sol(idx(lx, ly));
            val = std::clamp(val, 0.0, 255.0);
            auto pixel = result->get_pixel(tx, ty);
            const double alpha = (src_img_->channels() >= 4)
                             ? pixel_value_clamped(*src_img_, src_x(lx), src_y(ly), 3) /
                                           255.0
                                     : 1.0;
            const double blended =
                alpha * val + (1.0 - alpha) * static_cast<double>(pixel[c]);
            pixel[c] = static_cast<uchar>(std::clamp(
                static_cast<int>(std::lround(blended)), 0, 255));
            if (pixel.size() >= 4)
                pixel[3] = 255;
            result->set_pixel(tx, ty, pixel);
        }
    }
    return result;
}

// Solve with a different offset (realtime drag).
std::shared_ptr<Image> SeamlessClone::solve_with_new_offset(
    int new_offset_x,
    int new_offset_y)
{
    offset_x_ = new_offset_x;
    offset_y_ = new_offset_y;
    return solve();
}

}  // namespace USTC_CG
