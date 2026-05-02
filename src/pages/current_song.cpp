#include "current_song.hpp"
#include "display/display.hpp"
#include "data.hpp"


using namespace current_song;


void CurrentSongPage::draw_new()
{
    display::disp.draw_bg();
    display::disp.draw_info();
}


void CurrentSongPage::update()
{
    // check for new data
    if (data::s_data.new_thumbnail)
    {
        data::s_data.new_thumbnail = false;
        data::s_data.new_info = false;  // set to false because it gets drawn anyways

        // draw thumbnail
        display::disp.draw_bg();

        // re-draw info
        display::disp.draw_info();
    }
    else if (data::s_data.new_info)
    {
        data::s_data.new_info = false;

        // draw info
        display::disp.draw_info();
    }

    display::disp.update_progress();
}