/**
 * @file pages.hpp
 * @author Nilusink
 * @brief collection of all available pages
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include <Arduino.h>
#include "_base_page.hpp"
#include "current_song.hpp"
#include "volume_ctrl.hpp"


#define N_PAGES 2
#define CURRENT_SONG_PAGE 0
#define VOLUME_CTRL_PAGE 1


namespace pages
{
    extern size_t _current_page;


    extern current_song::CurrentSongPage _page0;
    extern volume_ctrl::VolumeCrtlPage _page1;

    extern base_page::BasePage *pages[N_PAGES];

    void next_page();
    void update_page();

    /**
     * @brief set page ID
     * 
     * @param page page ID
     */
    void set_page(size_t page);

    /**
     * @brief get current page ID
     * 
     * @return size_t page ID
     */
    size_t get_page();
} // namespace pages
