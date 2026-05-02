#include "encoder.hpp"
#include "pages/pages.hpp"


namespace encoder
{
    ESP32Encoder encoder;
} // namespace encoder


void encoder::setup()
{
    // setup pull-up resistors
    ESP32Encoder::useInternalWeakPullResistors = puType::up;

    // setup encoder
    encoder.attachHalfQuad(ENC_DT, ENC_CLK);
    encoder.clearCount();
}
