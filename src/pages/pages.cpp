#include "pages.hpp"
#include "display/display.hpp"


namespace pages
{
    size_t _current_page = 0;

    current_song::CurrentSongPage _page0;
    volume_ctrl::VolumeCrtlPage _page1;

    base_page::BasePage *pages[N_PAGES] = {
        &_page0,
        &_page1
    };
} // namespace pages



void pages::next_page()
{
    // change by 1
    _current_page = (_current_page+1) % N_PAGES;

    // call current page's draw_new
    display::disp.cls();
    pages[_current_page]->draw_new();
}


void pages::update_page()
{
    pages[_current_page]->update();
}


void pages::set_page(size_t page)
{
    _current_page = page;

    // call current page's draw_new
    display::disp.cls();
    pages[_current_page]->draw_new();
}


size_t pages::get_page()
{
    return _current_page;
}
