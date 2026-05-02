/**
 * @file current_song.hpp
 * @author Nilusink
 * @brief shows the currently played song
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include "_base_page.hpp"


namespace current_song
{
    class CurrentSongPage : public base_page::BasePage
    {
        public:
            CurrentSongPage() = default;

            void draw_new() override;
            void update() override;
    };
} // namespace pages
