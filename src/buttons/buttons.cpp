#include "buttons.hpp"


namespace buttons
{
    Button *skip_b;
    Button *pause_b;
    Button *next_b;
    Button *enc_b;
}

using namespace buttons;


Button::Button(uint8_t pin, bool inverted)
  : _pin(pin), _inverted(inverted)
{
    // setup pin
    pinMode(_pin, INPUT);

    // setup debouce stuff
    _value = digitalRead(_pin);

    _last_off = millis();
    _last_on = _last_off;
}


void Button::set_debounce_time(uint16_t t)
{
    _debounce_time = t;
}


void Button::update()
{
    // read current value
    bool cval = digitalRead(_pin);
    if (_inverted) cval = !cval;

    // set times and value
    unsigned long now = millis();
    if (cval)
    {
        _last_on = now;

        if (now - _last_off > _debounce_time)
            _value = true;
    }
    else
    {
        _last_off = now;

        if (now - _last_on > _debounce_time)
            _value = false;
    }
}


bool Button::get()
{
    return _value;
}


void buttons::setup()
{
    // setup buttons
    skip_b  = new Button(B0_PIN);
    pause_b = new Button(B1_PIN);
    next_b  = new Button(B2_PIN);
    enc_b   = new Button(ENC_SW, true);
}


void buttons::update()
{
    skip_b->update();
    pause_b->update();
    next_b->update();
    enc_b->update();
}
