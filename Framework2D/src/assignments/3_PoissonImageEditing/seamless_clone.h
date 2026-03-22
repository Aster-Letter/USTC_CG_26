#pragma once

#include <memory>
#include <vector>

#include <eigen/Eigen/Sparse>

#include "common/image.h"

namespace USTC_CG
{

enum class GradientMode
{
    kImporting = 0,  
    kMixing = 1      
};


class SeamlessClone
{
   public:
    // source   - source image (same size as mask)
    // target   - target / background image
    // mask     - binary mask (same size as source), 255 = selected
    // offset_x / offset_y - mouse_position - source_region_position
    SeamlessClone(
        std::shared_ptr<Image> source,
        std::shared_ptr<Image> target,
        std::shared_ptr<Image> mask,
        int offset_x,
        int offset_y,
        GradientMode mode = GradientMode::kImporting);

    // Pre-factorize matrix A (call once when the selected region changes).
    void precompute();

    // Solve with the current offset; calls precompute() on first use.
    std::shared_ptr<Image> solve();

    // Update offset and solve without re-factorizing A.
    std::shared_ptr<Image> solve_with_new_offset(int new_offset_x,
                                                  int new_offset_y);

   private:
    void build_matrix();
    Eigen::VectorXd build_rhs(int channel) const;

    int bbox_idx(int lx, int ly) const { return ly * region_w_ + lx; }
    int idx(int lx, int ly) const { return index_map_[bbox_idx(lx, ly)]; }
    bool inside_bbox(int lx, int ly) const;
    bool inside_mask(int lx, int ly) const;

    // Source pixel for local (lx, ly)
    int src_x(int lx) const { return src_min_x_ + lx; }
    int src_y(int ly) const { return src_min_y_ + ly; }
    // Target pixel for local (lx, ly)
    int tar_x(int lx) const { return src_min_x_ + lx + offset_x_; }
    int tar_y(int ly) const { return src_min_y_ + ly + offset_y_; }

    std::shared_ptr<Image> src_img_;
    std::shared_ptr<Image> tar_img_;
    std::shared_ptr<Image> mask_;

    // Offset from source coords to target coords
    int offset_x_, offset_y_;
    // Bounding box of selected region inside the mask/source image
    int src_min_x_ = 0, src_min_y_ = 0;
    int region_w_ = 0, region_h_ = 0;
    std::vector<int> index_map_;
    std::vector<std::pair<int, int>> active_pixels_;

    GradientMode mode_;

    Eigen::SparseMatrix<double> A_;
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver_;
    bool factorized_ = false;
};

}  // namespace USTC_CG
