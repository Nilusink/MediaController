#include "display.hpp"


/* @section: logic only */
uint16_t display::average565(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
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

uint16_t sample_2x(int sx, int sy, uint16_t* src, int w, int h)
{
    sx = constrain(sx, 0, w - 1);
    sy = constrain(sy, 0, h - 1);
    return src[sy * w + sx];
}

void renderLine2xBlur(
    int outY,
    uint16_t *src,
    uint16_t *outLine
)
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

        outLine[x] = display::average565(c1, c2, c3, c4);
    }
}

int text_width(const char* str, int textSize)
{
    return strlen(str) * 6 * textSize;
}

uint16_t display::rgbTo565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}
/* @endsection */


namespace display
{
    _Display disp;
} // namespace display



using namespace display;


/* @section: util */
uint8_t _Display::get_line_start(uint8_t row)
{
    for (uint8_t i=0; i < SRC_W; i++)
    {
        if (data::s_data.image_buffer[(SRC_W * row) + i] > 0)
        {
            return i;
        }
    }
    return 255;
}


uint8_t _Display::get_line_end(uint8_t row)
{
    for (int i = SRC_W - 1; i >= 0; i--)
    {
        if (data::s_data.image_buffer[(SRC_W * row) + i] > 0)
        {
            return (i > SRC_W) ? SRC_W : i;  // clamp to max SRC_W
        }
    }
    return SRC_W;
}
/* @endsection */


/* @section: circle stuff */
void _Display::renderImageLineByLine(uint16_t* src, int w, int h)
{
    for (int y = 0; y < OUT_W; y++)
    {
        renderLine2xBlur(y, src, line_buff);

        // copy to progress buffer:
        if (y >= PROG_Y && y < PROG_Y + PROG_HEIGHT)
        {
            memcpy(
                &progress_bar_bg[PROG_LEN * (y - PROG_Y)],
                &line_buff[(OUT_W - PROG_LEN) / 2],
                PROG_LEN * sizeof(uint16_t)
            );
        }

        display.drawRGBBitmap(
            1, y+1,
            line_buff,
            OUT_W, 1
        );
    }
}


void _Display::draw_circle_bg()
{
    for (int y = 0; y < SRC_H; y++)
    {
        uint8_t row_start = get_line_start(y);
        uint8_t row_end = get_line_end(y);

        uint8_t row_len = row_end - row_start;
        if (row_len <= 0 || row_len > SRC_W)
            continue;

        uint8_t dest_x = 80 + row_start;   // use source start, not derived width center
        uint8_t dest_y = 80 + y;

        display.drawRGBBitmap(
            dest_x,
            dest_y,
            &data::s_data.image_buffer[row_start + SRC_W * y],
            row_len,
            1
        );
    }
}
/* @endsection */


_Display::_Display()
  : spi(FSPI), display(&spi, TFT_DC, TFT_CS, TFT_RST)
{
    spi.begin(
        TFT_SCL,   // SCK
        -1,        // MISO (not used)
        TFT_SDA,   // MOSI
        TFT_CS     // CS (optional but fine)
    );

    display.begin();
    display.setSPISpeed(SPI_FREQUENCY);  // 40 MHz

    // screen orientation
    display.setRotation(1);

    // fill screen with black
    display.fillScreen(GC9A01A_BLACK);

    // logic init
    data::s_data.last_update = millis();
}


/* @section: public functions */
void _Display::draw_bg()
{
    // draw bg
    renderImageLineByLine(data::s_data.image_buffer, 240, 240);

    // draw little image
    draw_circle_bg();
}


void _Display::draw_info()
{
    auto_text(data::s_data.title.c_str(), 60, rgbTo565(255, 255, 255), 200, 3);
    // auto_text(album.c_str(), 55, rgbTo565(180, 180, 180), 200, 2);
    auto_text(data::s_data.artist.c_str(), 176, rgbTo565(200, 200, 200), 200, 2);
}


void _Display::auto_text(
    const char* str,
    uint8_t y_pos,
    uint16_t color,
    uint8_t max_length,
    uint8_t start_size
)
{
    // set font size
    uint16_t text_len = 0;
    for (; start_size > 1; start_size--)
    {
        text_len = text_width(str, start_size);

        if (text_len <= max_length)
            break;
    }
    text_len = text_width(str, start_size);

    // calculate text position
    uint8_t x_pos = (240 - text_len) / 2;
    uint8_t char_height = (8*start_size);

    // render text
    display.setTextSize(start_size);
    display.setTextColor(color);

    if (text_len < max_length)
    {
        display.setCursor(x_pos, y_pos - char_height / 2);  // center text
        display.print(str);
    }
    else
    {   // text is still too long, split up  strlen(str) * 6 * textSize;
        uint8_t char_length = (6*start_size);
        uint8_t max_chars = max_length / char_length;

        // allow min 1 character
        if (max_chars == 0) max_chars = 1;

        uint8_t n_splits = (strlen(str) + max_chars - 1) / max_chars;

        // print lines
        double curr_y;
        char buff[256];
        for (uint8_t split = 0; split < n_splits; split++)
        {
            // slice string
            if (split < n_splits-1)
            {
                strncpy(buff, &str[max_chars*split], max_chars);
                buff[max_chars] = '\0';

                x_pos = (TFT_WIDTH - max_length) / 2;
            }
            else
            {
                uint8_t len = strnlen(&str[max_chars*split], max_chars);
                strncpy(buff, &str[max_chars*split], len);
                buff[len] = '\0';

                x_pos = (TFT_WIDTH - len) / 2;
            }

            // set cursor centered around y_pos
            curr_y = y_pos + (split - (n_splits - 1) / 2.0) * (char_height + 2);
            display.setCursor(x_pos, curr_y);

            // print line
            display.print(buff);
        }
    }
}


void _Display::clear_background(uint8_t x0, uint8_t y0, uint8_t width, uint8_t height)
{
    display.fillRect(
        x0, y0, width, height, GC9A01A_BLACK
    );
}

void _Display::cls()
{
    // reset progress bar buffer
    memset(progress_bar_bg, 0, sizeof(progress_bar_bg));

    // clear screen
    display.fillScreen(GC9A01A_BLACK);
}

void _Display::update_progress()
{
    if (data::s_data.duration > 0)
    {
        float delta = (millis() - data::s_data.last_update) / 1000.f;

        const float centerY = PROG_HEIGHT * 0.5f;

        uint16_t white = rgbTo565(255, 255, 255);
        uint16_t gray  = rgbTo565(128, 128, 128);
        uint16_t bg    = rgbTo565(0, 0, 0);

        float now = millis() * 0.001f;

        memcpy(
            &progress_bar_buffer,
            &progress_bar_bg,
            PROG_LEN * PROG_HEIGHT * sizeof(uint16_t)
        );

        // non-wave mode
        float t;
        if (data::s_data.status != 4)
        {
            t = data::s_data.position / data::s_data.duration;
            int split = (int)(PROG_LEN * t);

            for (int x = 0; x < PROG_LEN; x++)
            {
                uint16_t col = (x <= split) ? white : gray;

                progress_bar_buffer[PROG_HEIGHT / 2 * PROG_LEN + x] = col;
            }
        }
        else
        {
            t = (data::s_data.position + delta) / data::s_data.duration;

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
            uint8_t px = split;
            uint8_t py = (int)waveY(split);

            uint8_t r = 6;
            for (int y = -r; y <= r; y++)
            {
                for (int x = -r; x <= r; x++)
                {
                    if (x*x + y*y <= r*r)
                    {
                        uint8_t dx = px + x;
                        uint8_t dy = py + y;

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

    // draw buffer to screen
    display.drawRGBBitmap(
        (240-PROG_LEN)/2, PROG_Y,
        progress_bar_buffer,
        PROG_LEN,
        PROG_HEIGHT
    );
}
/* @endsection */


/* @section: manual draw */
void _Display::draw_outer_circle(
    uint8_t width,
    uint16_t color,
    uint16_t bg_color,
    uint8_t outer_radius,
    uint8_t x,
    uint8_t y
)
{
    // draw outer circle
    display.fillCircle(x, y, outer_radius, color);

    // draw inner circle
    display.fillCircle(x, y, outer_radius-width, bg_color);
}

void _Display::draw_arc(
    int cx, int cy,
    int radius,
    int thickness,
    float start_deg,
    float end_deg,
    uint16_t color
)
{
    if (end_deg < start_deg) {
        float tmp = start_deg;
        start_deg = end_deg;
        end_deg = tmp;
    }

    int inner = radius - thickness;
    if (inner < 0) inner = 0;

    // step controls smoothness vs speed
    float step = .004f;

    for (float a = start_deg; a <= end_deg; a += step)
    {
        float ca = cosf(a);
        float sa = sinf(a);

        // draw radial thickness
        for (int r = inner; r <= radius; r++)
        {
            int x = cx + (int)(ca * r);
            int y = cy + (int)(sa * r);
            display.drawPixel(x, y, color);
        }
    }
}
/* @endsection */
