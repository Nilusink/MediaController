/**
 * @file volume_ctrl.hpp
 * @author Nilusink
 * @brief volume settings
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include "_base_page.hpp"


namespace volume_ctrl
{
    class VolumeCrtlPage : public base_page::BasePage
    {
        public:
            VolumeCrtlPage() = default;

            void draw_new() override;
            void update() override;
    };
} // namespace volume_ctrl
