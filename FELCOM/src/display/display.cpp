#include "display.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setupDisplay()
{
    Wire.setPins(SCREEN_SDA_PIN, SCREEN_SCL_PIN);
    Wire.begin();
    if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR))
    {
        LOG_ERROR("SSD1306 allocation failed");
        return;
    }
    display.clearDisplay();
}
