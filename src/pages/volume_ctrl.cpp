#include "volume_ctrl.hpp"
#include "display/display.hpp"
#include "data.hpp"


using namespace volume_ctrl;


void VolumeCrtlPage::draw_new()
{
}


void VolumeCrtlPage::update()
{
    double circ_len = data::v_data.volume / 100;

    // cap to 0-1
    if (0 > circ_len) circ_len = 0;
    if (1 < circ_len) circ_len = 1;

    display::disp.draw_arc(
        TFT_WIDTH / 2,
        TFT_HEIGHT / 2,
        TFT_WIDTH / 2,
        10,
        M_PI_2,
        circ_len * M_TWOPI + M_PI_2,
        0xFFFF
    );
}
