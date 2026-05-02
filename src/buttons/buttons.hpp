/**
 * @file buttons.hpp
 * @author Nilusink
 * @brief software de-bounced buttons
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include <Arduino.h>


namespace buttons
{
    class Button
    {
        private:
            uint8_t _pin;
            bool _inverted, _value;

            // debounce stuff
            uint16_t _debounce_time = 20;  // 20 ms
            unsigned long _last_off;
            unsigned long _last_on;
        
        public:
        Button(uint8_t pin, bool inverted = true);

        /**
         * @brief Set the time the button needs to be one specific value
         * 
         * @param t time in ms
         */
        void set_debounce_time(uint16_t t);

        /**
         * @brief update the button
         * 
         */
        void update();

        /**
         * @brief get current button value
         * 
         */
        bool get();
    };

    extern Button *skip_b;
    extern Button *pause_b;
    extern Button *next_b;
    extern Button *enc_b;

    void setup();
    void update();
} // namespace buttons
