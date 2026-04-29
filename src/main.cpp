#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#define SEP "<|>"
#define SEP_LEN 3
#define PROG_LEN 170
#define PROG_HEIGHT 20
#define PROG_Y 195

#define PROG_START (120 - PROG_LEN / 2)
#define PROG_END (120 + PROG_LEN / 2)
#define PROG_CENTER_Y (PROG_Y + PROG_HEIGHT / 2)

#define SRC_W 80
#define SRC_H 80

#define OUT_W 240
#define OUT_H 240

#define SCALE_FAC 3

// Create display instance
Adafruit_GC9A01A display = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST);


// display variables
unsigned long last_pos_update;
uint8_t playback_status;
float position, duration;

uint16_t lineBuf[OUT_W];
uint16_t image_buffer[SRC_W * SRC_H] = {0};
uint16_t progress_bar_bg[PROG_LEN * PROG_HEIGHT] = {0};
uint16_t progress_bar_buffer[PROG_LEN * PROG_HEIGHT] = {0};


int text_width(const char* str, int textSize)
{
    return strlen(str) * 6 * textSize;
}


static inline uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}


void clear_background(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height)
{
    display.fillRect(
        x0, y0, width, height, GC9A01A_BLACK
    );
}


uint8_t x_pos;
void auto_text(
    const char* str,
    uint8_t y_pos,
    uint16_t color = GC9A01A_WHITE,
    uint8_t max_length = 200,
    uint8_t start_size = 3
)
{
    // set font size
    uint16_t text_len = 0;
    for (; start_size > 0; start_size--)
    {
        text_len = text_width(str, start_size);

        if (text_len <= max_length)
            break;
    }

    // calculate text position
    x_pos = (uint8_t)((240 - text_len) / 2);

    // render text
    display.setTextSize(start_size);
    display.setTextColor(color);
    display.setCursor(x_pos, y_pos);
    display.print(str);
}


uint8_t get_line_start(uint8_t row)
{
    for (uint8_t i=0; i < SRC_W; i++)
    {
        if (image_buffer[(SRC_W * row) + i] > 0)
        {
            return i;
        }
    }
    return 255;
}


uint8_t get_line_end(uint8_t row)
{
    for (uint8_t i=SRC_W-1; i >= 0; i--)
    {
        if (image_buffer[(SRC_W * row) + i] > 0)
        {
            return i;
        }
    }
    return SRC_W;
}


uint16_t sample_2x(int sx, int sy, uint16_t* src, int w, int h)
{
    sx = constrain(sx, 0, w - 1);
    sy = constrain(sy, 0, h - 1);
    return src[sy * w + sx];
}


inline uint16_t sample_80(uint16_t* src, int w, int h, int x, int y)
{
    x = constrain(x, 0, w - 1);
    y = constrain(y, 0, h - 1);
    return src[y * w + x];
}


uint16_t average565(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
    uint16_t r = (((a >> 11) & 0x1F) +
                  ((b >> 11) & 0x1F) +
                  ((c >> 11) & 0x1F) +
                  ((d >> 11) & 0x1F)) / (4 * 3);

    uint16_t g = (((a >> 5) & 0x3F) +
                  ((b >> 5) & 0x3F) +
                  ((c >> 5) & 0x3F) +
                  ((d >> 5) & 0x3F)) / (4 * 3);

    uint16_t bch = ((a & 0x1F) +
                    (b & 0x1F) +
                    (c & 0x1F) +
                    (d & 0x1F)) / (4 * 3);

    return (r << 11) | (g << 5) | bch;
}


void renderLine2xBlur(int outY,
                       uint16_t* src,
                       uint16_t* outLine)
{
    int sy = outY / SCALE_FAC;

    for (int x = 0; x < OUT_W; x++)
    {
        int sx = x / SCALE_FAC;

        // clamp helper (important at edges)
        int sx1 = (sx + 1 < SRC_W) ? sx + 1 : sx;
        int sy1 = (sy + 1 < SRC_H) ? sy + 1 : sy;

        uint16_t c1 = src[sy * SRC_W + sx];
        uint16_t c2 = src[sy * SRC_W + sx1];
        uint16_t c3 = src[sy1 * SRC_W + sx];
        uint16_t c4 = src[sy1 * SRC_W + sx1];

        outLine[x] = average565(c1, c2, c3, c4);
    }
}


uint8_t line_start;
uint8_t renderLine80(int y, uint16_t* src, uint16_t* out)
{
    int sy = y * SRC_H / 80;

    line_start = 0;
    for (int x = 0; x < 80; x++)
    {
        int sx = x * SRC_W / 80;

        out[x] = sample_80(src, SRC_W, SRC_H, sx, sy);

        if (x <= 40 && out[x] == 0 && line_start == 0)
        {
            line_start = x;
        }
    }

    return 80 - 2*line_start;
}


void renderImageLineByLine(uint16_t* src, int w, int h)
{
    for (int y = 0; y < OUT_W; y++)
    {
        renderLine2xBlur(y, src, lineBuf);

        // copy to progress buffer:
        if (y >= PROG_Y && y < PROG_Y + PROG_HEIGHT)
        {
            memcpy(
                &progress_bar_bg[PROG_LEN * (y - PROG_Y)],
                &lineBuf[(OUT_W - PROG_LEN) / 2],
                PROG_LEN * sizeof(uint16_t)
            );
        }

        display.drawRGBBitmap(
            0, y,
            lineBuf,
            OUT_W, 1
        );
    }
}


uint8_t row_len, ls;
void draw_circle_bg()
{
    for (int y = 0; y < SRC_H; y++)
    {
        row_len = get_line_start(y);

        if (row_len >= SRC_W)
            continue;
        
        row_len = get_line_end(y) - row_len;

        ls = 80 + (SRC_W-row_len) / 2;
        display.drawRGBBitmap(
            ls,
            80 + y,
            &image_buffer[SRC_W * y + ls],
            row_len,
            1
        );
    }
}

void draw_bg()
{
    // draw bg
    renderImageLineByLine(image_buffer, 240, 240);

    // draw little image
    draw_circle_bg();
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
            String artist = msg.substring(sep_0 + SEP_LEN, sep_1);
            String title = msg.substring(sep_1 + SEP_LEN, sep_2);
            String album = msg.substring(sep_2 + SEP_LEN);
            
            // update display
            // clear_background(0, 30, 240, 75);
            // clear_background(0, 170, 240, 190);

            auto_text(title.c_str(), 40, rgbTo565(255, 255, 255), 180, 3);
            // auto_text(album.c_str(), 55, rgbTo565(180, 180, 180), 200, 2);

            auto_text(artist.c_str(), 170, rgbTo565(200, 200, 200), 200, 2);

            // confirm OK
            Serial.println("OK");
        } else {
            Serial.println("FAIL");
        }
    }
    else if (msg.startsWith("PRG<|>"))
    {
        sep_0 = msg.indexOf(SEP);
        sep_1 = msg.indexOf(SEP, sep_0 + SEP_LEN);
        sep_2 = msg.indexOf(SEP, sep_1 + SEP_LEN);

        if (sep_0 != -1 && sep_1 != -1 && sep_2 != -1)
        {
            last_pos_update = millis();

            // get values
            String pos = msg.substring(sep_0 + SEP_LEN, sep_1);
            String dur = msg.substring(sep_1 + SEP_LEN, sep_2);
            String status = msg.substring(sep_2 + SEP_LEN);

            // convert to float
            position = pos.toFloat();
            duration = dur.toFloat();
            playback_status = status.toInt();

            // confirm OK
            Serial.println("OK");
        } else {
            Serial.println("FAIL");
        }
    }
    else if (msg.startsWith("IMG<|>"))
    {
        // receive image
        Serial.readBytes((char*)image_buffer, SRC_W * SRC_W * 2);

        // draw image
        draw_bg();

        // send ACK
        Serial.println("OK");
    }
    else if (msg.startsWith("CLS<|>"))
    {
        // reset progress bar buffer
        memset(progress_bar_bg, 0, sizeof(progress_bar_bg));

        // clear screen
        display.fillScreen(GC9A01A_BLACK);

        // send ACK
        Serial.println("OK");
    }
    else
    {
        Serial.print("FAIL, "); Serial.println(msg);
    }
}


void setup()
{
    last_pos_update = millis();
    Serial.begin(115200);

    display.begin();

    // screen orientation
    display.setRotation(2);

    // fill screen with black
    display.fillScreen(GC9A01A_BLACK);
}


float delta, t;
uint8_t prog_pos, r, dx, dy, px, py;
void update_progress()
{
    if (duration > 0)
    {
        delta = (millis() - last_pos_update) / 1000.f;

        const float centerY = PROG_HEIGHT * 0.5f;

        uint16_t white = rgbTo565(255, 255, 255);
        uint16_t gray  = rgbTo565(128, 128, 128);
        uint16_t bg    = rgbTo565(0, 0, 0);

        float now = millis() * 0.001f;

        // clear buffer first (required since no alpha like pygame)
        // for (int i = 0; i < PROG_LEN * PROG_HEIGHT; i++)
        //     progress_bar_buffer[i] = progress_bar_bg[i];

        memcpy(
            &progress_bar_buffer,
            &progress_bar_bg,
            PROG_LEN * PROG_HEIGHT * sizeof(uint16_t)
        );

        // non-wave mode
        if (playback_status != 4)
        {
            t = position / duration;
            int split = (int)(PROG_LEN * t);

            for (int x = 0; x < PROG_LEN; x++)
            {
                uint16_t col = (x <= split) ? white : gray;

                progress_bar_buffer[PROG_HEIGHT / 2 * PROG_LEN + x] = col;
            }

            return;
        }

        t = (position + delta) / duration;

        // wave mode
        float amp  = PROG_HEIGHT * 0.20f;
        float freq = 4.0f;
        float phase = now * 1.0f;

        int split = (int)(PROG_LEN * t);

        auto waveY = [&](int x)
        {
            float fx = (float)x / (float)PROG_LEN;
            return centerY + sinf(fx * freq * 2.0f * M_PI + phase) * amp;
        };

        for (int x = 0; x < PROG_LEN; x++)
        {
            int y = (int)waveY(x);

            if (y < 0 || y >= PROG_HEIGHT) continue;

            uint16_t col = (x <= split) ? white : gray;

            progress_bar_buffer[y * PROG_LEN + x] = col;
        }

        // progress "dot"
        px = split;
        py = (int)waveY(split);

        r = 6;
        for (int y = -r; y <= r; y++)
        {
            for (int x = -r; x <= r; x++)
            {
                if (x*x + y*y <= r*r)
                {
                    dx = px + x;
                    dy = py + y;

                    if (dx >= 0 && dx < PROG_LEN &&
                        dy >= 0 && dy < PROG_HEIGHT)
                    {
                        progress_bar_buffer[dy * PROG_LEN + dx] = white;
                    }
                }
            }
        }
    }
}


void loop() {
    if (Serial.available()) {
        String msg = Serial.readStringUntil('\n');
        process_message(msg);
    }

    // update progress bar
    update_progress();

    display.drawRGBBitmap(
        (240-PROG_LEN)/2, PROG_Y,
        progress_bar_buffer,
        PROG_LEN,
        PROG_HEIGHT
    );
}
