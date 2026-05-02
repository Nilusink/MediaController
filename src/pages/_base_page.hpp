/**
 * @file _base_page.hpp
 * @author Nilusink
 * @brief defines page interfaces
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once


namespace base_page
{
    class BasePage
    {
        public:
            BasePage() = default;
            virtual ~BasePage() = default;

            /**
             * @brief called when page is first drawn
             * 
             */
            virtual void draw_new() = 0;

            /**
             * @brief called while actively being shown
             * 
             */
            virtual void update() = 0;
    };
} // namespace base_page
