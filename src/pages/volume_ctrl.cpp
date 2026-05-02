#include "volume_ctrl.hpp"
#include "display/display.hpp"
#include "data.hpp"


using namespace volume_ctrl;


void VolumeCrtlPage::draw_new()
{
    // draw volume text
    last_drawn_volume = -1;
    update();

    // draw text
    display::disp.auto_text(
        "Volume",
        100,
        display::rgbTo565(200, 200, 200),
        200,
        3
    );

    // draw arc
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


void VolumeCrtlPage::update()
{
    double circ_start = (data::v_data.volume - (data::v_data.delta * 1.1)) / 100;
    double circ_end = data::v_data.volume / 100;
    data::v_data.delta = 0;

    // cap to 0-1
    if (0 > circ_start) circ_start = 0;
    if (1 < circ_start) circ_start = 1;

    if (0 > circ_end) circ_end = 0;
    if (1 < circ_end) circ_end = 1;

    // draw volume circle
    if (circ_end > circ_start)
    {
        display::disp.draw_arc(
            TFT_WIDTH / 2,
            TFT_HEIGHT / 2,
            TFT_WIDTH / 2,
            10,
            circ_start * M_TWOPI + M_PI_2,
            circ_end * M_TWOPI + M_PI_2,
            0xFFFF
        );
    }
    else
    {
        display::disp.draw_arc(
            TFT_WIDTH / 2,
            TFT_HEIGHT / 2,
            TFT_WIDTH / 2,
            12,
            circ_end * M_TWOPI + M_PI_2,
            circ_start * M_TWOPI + M_PI_2,
            0
        );
    }

    // Draw text
    if (data::v_data.volume != last_drawn_volume)
    {
        char buff[8];
        snprintf(buff, 8, "%.0f %%", data::v_data.volume);

        display::disp.clear_background(50, 120, 140, 40);
        display::disp.auto_text(
            buff,
            140,
            display::rgbTo565(200, 200, 200),
            200,
            3
        );

        last_drawn_volume = data::v_data.volume;
    }
}
