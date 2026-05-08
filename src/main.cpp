#include <Arduino.h>
#include <ESP32Encoder.h>
#include "display/display.hpp"
#include "buttons/buttons.hpp"
#include "encoder/encoder.hpp"
#include "pages/pages.hpp"

#define SEP "<|>"
#define SEP_LEN 3
#define CHUNK 1024


// disp variables
unsigned long last_input;
bool curr_btn, last_btn = 0;


uint32_t checksum32(const uint8_t* data, size_t len)
{
    uint32_t sum = 0;

    for (size_t i = 0; i < len; i++)
    {
        sum += data[i];
    }

    return sum;
}


uint8_t sep_0, sep_1, sep_2;
void process_message(const String& msg)
{
    if (msg.startsWith("INFO<|>"))
    {
        // Split the message into parts
        sep_0 = msg.indexOf(SEP);
        sep_1 = msg.indexOf(SEP, sep_0 + SEP_LEN);
        sep_2 = msg.indexOf(SEP, sep_1 + SEP_LEN);
        
        if (sep_0 != -1 && sep_1 != -1 && sep_2 != -1)
        {
            data::s_data.artist = msg.substring(sep_0 + SEP_LEN, sep_1);
            data::s_data.title = msg.substring(sep_1 + SEP_LEN, sep_2);
            data::s_data.album_title = msg.substring(sep_2 + SEP_LEN);
            
            // confirm OK
            Serial.println("OK");
        } else {
            Serial.println("FAIL");
        }

        // update display
        data::s_data.new_info = true;
    }
    else if (msg.startsWith("PRG<|>"))
    {
        sep_0 = msg.indexOf(SEP);
        sep_1 = msg.indexOf(SEP, sep_0 + SEP_LEN);
        sep_2 = msg.indexOf(SEP, sep_1 + SEP_LEN);

        if (sep_0 != -1 && sep_1 != -1 && sep_2 != -1)
        {
            data::s_data.last_update = millis();

            // get values
            String pos = msg.substring(sep_0 + SEP_LEN, sep_1);
            String dur = msg.substring(sep_1 + SEP_LEN, sep_2);
            String status = msg.substring(sep_2 + SEP_LEN);

            // convert to float
            data::s_data.position = pos.toFloat();
            data::s_data.duration = dur.toFloat();
            data::s_data.status = status.toInt();

            // confirm OK
            Serial.println("OK");
        }
        else
        {
            Serial.println("FAIL");
        }
    }
    else if (msg.startsWith("IMG<|>"))
    {
        // receive image
        unsigned long start = millis();
        uint32_t size, expected_cs, n;

        Serial.readBytes((char*)&size, 4);
        Serial.readBytes((char*)&expected_cs, 4);

        // receive data
        n = Serial.readBytes((char*)data::s_data.image_buffer, SRC_W * SRC_H * 2);

        // compute checksum
        uint32_t actual_cs = checksum32((uint8_t*)data::s_data.image_buffer, size);

        // send ACK
        data::s_data.new_thumbnail = true;
        if (n < size)
        {
            Serial.print("FAIL: size="); Serial.println(size);

            // clear image buffer
            memset(data::s_data.image_buffer, 0, SRC_W * SRC_H * 2);
        }
        else if (actual_cs == expected_cs)
        {
            Serial.println("OK");
        }
        else
        {
            Serial.print("FAIL: CS="); Serial.println(actual_cs);

            // clear image buffer
            memset(data::s_data.image_buffer, 0, SRC_W * SRC_H * 2);
        }
    }
    else if (msg.startsWith("CLS<|>"))
    {
        display::disp.cls();

        // send ACK
        Serial.println("OK");
    }
    else if (msg.startsWith("VOL<|>"))
    {
        // set volume
        sep_0 = msg.indexOf(SEP);

        if (sep_0 != -1)
        {
            // convert msg to float
            String volume = msg.substring(sep_0 + SEP_LEN);
            data::v_data.volume = volume.toFloat();

            // confirm OK
            Serial.println("OK");

            // change page
            pages::set_page(VOLUME_CTRL_PAGE);
            last_input = millis();
        }
        else
        {
            Serial.println("FAIL");
        }

    }
    else
    {
        Serial.print("FAIL, "); Serial.println(msg);
    }
}


void send_volume()
{
    char buff[16];
    snprintf(buff, 16, "VOL%s%d", SEP, (uint8_t)data::v_data.volume);

    Serial.println(buff);
}

void send_toggle_prev()
{
    Serial.println("PRE");
}

void send_toggle_play()
{
    Serial.println("PLY");
}

void send_toggle_skip()
{
    Serial.println("SKP");
}


void setup()
{
    Serial.begin(115200);
    Serial.setTimeout(5000);

    // setup stuff
    buttons::setup();
    encoder::setup();

    // set button callbacks
    buttons::next_b->set_callback(&send_toggle_prev);
    buttons::pause_b->set_callback(&send_toggle_play);
    buttons::skip_b->set_callback(&send_toggle_skip);
}


unsigned long now;
void loop() {
    if (Serial.available()) {
        String msg = Serial.readStringUntil('\n');
        process_message(msg);
    }

    // update logic
    buttons::update();

    now = millis();

    // handle menu
    curr_btn = buttons::enc_b->get();
    if (curr_btn && !last_btn)
    {
        pages::next_page();
        
        last_input = now;
    }

    // handle encoder stuff
    if (encoder::encoder.getCount() != 0)
    {
        last_input = now;

        // get data
        double count = encoder::encoder.getCount() * 2.5;
        encoder::encoder.clearCount();

        if (!(count > 0 && data::v_data.volume >= 100) && !(count < 0 && data::v_data.volume <= 0))
        {
            //  when close to 100 or 0, clamp count
            if (data::v_data.volume + count > 100)
            {
                count = 100 - data::v_data.volume;
            }
            else if (data::v_data.volume + count < 0)
            {
                count = 0 - data::v_data.volume;
            }

            data::v_data.volume += count;
            data::v_data.delta = count;

            // update pc volume
            send_volume();
        }

        // set page
        pages::set_page(VOLUME_CTRL_PAGE);
    }

    // if last input > 5s, quit menu
    if (pages::get_page() > 0 && now - last_input > 5000)
    {
        pages::set_page(CURRENT_SONG_PAGE);
    }

    // update current page
    pages::update_page();

    last_btn = curr_btn;
}
