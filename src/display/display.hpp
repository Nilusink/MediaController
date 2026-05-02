/**
 * @file display.hpp
 * @author Nilusink
 * @brief display manager
 * @version 0.1
 * @date 2026-04-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include "data.hpp"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#define PROG_LEN 170
#define PROG_HEIGHT 20
#define PROG_Y 195

#define PROG_START (120 - PROG_LEN / 2)
#define PROG_END (120 + PROG_LEN / 2)
#define PROG_CENTER_Y (PROG_Y + PROG_HEIGHT / 2)

#define SCALE_FAC 3


namespace display
{
    uint16_t average565(uint16_t a, uint16_t b, uint16_t c, uint16_t d);
    uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b);

    class _Display
    {
        private:
            uint16_t line_buff[OUT_W] = {0};
            uint16_t progress_bar_bg[PROG_LEN * PROG_HEIGHT] = {0};
            uint16_t progress_bar_buffer[PROG_LEN * PROG_HEIGHT] = {0};

            SPIClass spi;
            Adafruit_GC9A01A display;
        
        protected:
            uint8_t get_line_start(uint8_t row);
            uint8_t get_line_end(uint8_t row);

            void renderImageLineByLine(uint16_t* src, int w, int h);
            void draw_circle_bg();

        public:
            _Display();

            /**
             * @brief draw image background
             * 
             */
            void draw_bg();

            /**
             * @brief draw song info
             * 
             */
             void draw_info();

            /**
             * @brief draw text x-centered at y_pos with max size and variable font size
             * 
             * @param str text
             * @param y_pos y position of text
             * @param color text color
             * @param max_length max x length
             * @param start_size starting font size
             */
            void auto_text(
                const char* str,
                uint8_t y_pos,
                uint16_t color = GC9A01A_WHITE,
                uint8_t max_length = 200,
                uint8_t start_size = 3
            );

            /**
             * @brief clear background at specified position
             * 
             */
            void clear_background(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height);

            /**
             * @brief clear screen
             * 
             */
            void cls();

            /**
             * @brief update progress bar
             * 
             */
            void update_progress();

            void draw_outer_circle(
                uint8_t width,
                uint16_t color,
                uint16_t bg_color = 0,
                uint8_t outer_radius = TFT_WIDTH / 2,
                uint8_t x = TFT_WIDTH / 2,
                uint8_t y = TFT_WIDTH / 2
            );

            void draw_arc(
                int cx, int cy,          // center
                int radius,              // outer radius
                int thickness,           // arc width
                float start_deg,         // start angle in rad
                float end_deg,           // end angle in rad
                uint16_t color
            );
    };

    extern _Display disp;
} // namespace display
