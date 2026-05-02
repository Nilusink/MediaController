/**
 * @file encoder.hpp
 * @author Nilusink
 * @brief encoder stuff
 * @version 0.1
 * @date 2026-05-02
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include <ESP32Encoder.h>
#include "buttons/buttons.hpp"


namespace encoder
{
    extern ESP32Encoder encoder;

    void setup();

} // namespace encoder
