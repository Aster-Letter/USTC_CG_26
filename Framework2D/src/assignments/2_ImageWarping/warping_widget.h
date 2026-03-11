#pragma once

#include <utility>

#include "common/image_widget.h"

namespace USTC_CG
{
// Image component for warping and other functions
class WarpingWidget : public ImageWidget
{
   public:
    explicit WarpingWidget(
        const std::string& label,
        const std::string& filename);
    virtual ~WarpingWidget() noexcept = default;

    void draw() override;

    // Simple edit functions
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale();
    void warping();
    void restore();

    // Enumeration for supported warping types.
    // HW2_TODO: more warping types.
    enum WarpingType
    {
        kDefault = 0,
        kFisheye = 1,
        kIDW = 2,
        kRBF = 3,
    };
    // Warping type setters.
    void set_default();
    void set_fisheye();
    void set_IDW();
    void set_RBF();
    void set_fisheye_strength(float strength);
    void set_idw_power(float power);
    void set_rbf_radius_scale(float scale);
    void reset_warping_parameters();
    [[nodiscard]] float fisheye_strength() const;
    [[nodiscard]] float idw_power() const;
    [[nodiscard]] float rbf_radius_scale() const;

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;

    ImVec2 start_, end_;
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;
    WarpingType warping_type_;
    float fisheye_strength_ = 10.0f;
    float idw_power_ = 2.0f;
    float rbf_radius_scale_ = 1.0f;
    float background_gray_ = 96.0f;

   private:
    std::pair<double, double> fisheye_inverse_warping(
        double x,
        double y,
        int width,
        int height) const;
};

}  // namespace USTC_CG