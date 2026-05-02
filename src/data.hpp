/**
 * @file data.hpp
 * @author Nilusink
 * @brief data struct
 * @version 0.1
 * @date 2026-04-30
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once
#include <Arduino.h>


#define SRC_W 80
#define SRC_H 80

#define OUT_W 240
#define OUT_H 240


namespace data
{
    // structs
    struct _SongData
    {
        bool new_thumbnail = false;  // for re-drawing
        bool new_info = false;
        uint8_t status;
        float position;
        float duration;
        unsigned long last_update;
        uint16_t image_buffer[SRC_W * SRC_H];
        String title, artist, album_title;
    };

    struct _VolumeData
    {
        float volume = 50;
    };

    // instances
    extern _SongData s_data;
    extern _VolumeData v_data;
} // namespace data
