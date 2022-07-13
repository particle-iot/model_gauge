
#include "Particle.h"
#include "model_gauge.h"

SerialLogHandler logHandler(115200, LOG_LEVEL_INFO);
SYSTEM_MODE(MANUAL);

// define your new battery model config first, or use the default config in model_gauge.h
// example: LG INR1865 1S4P battery model get from the .ini file
const model_config_t model_config_example = {
    .EmptyAdjustment=0,
    .FullAdjustment=100,
    .RCOMP0 = 123,
    .TempCoUp = -0.0,
    .TempCoDown = -0.0,
    .OCVTest = 56176,
    .SOCCheckA = 225,
    .SOCCheckB = 227,
    .bits = 19,
    .model_data = {
        0x99, 0x20, 0xA6, 0xA0, 0xA9, 0x50, 0xAC, 0x40, 0xB0, 0x60, 0xB3, 0x20, 0xB4, 0xF0, 0xB7, 0x60,
        0xBB, 0xF0, 0xBE, 0xC0, 0xC2, 0x00, 0xC5, 0x50, 0xC8, 0xF0, 0xCB, 0x10, 0xCD, 0x10, 0xD1, 0x70,
        0x01, 0x20, 0x14, 0x40, 0x0A, 0xA0, 0x0C, 0x40, 0x1A, 0x00, 0x23, 0x20, 0x1D, 0xE0, 0x0F, 0xA0,
        0x0A, 0x60, 0x13, 0x80, 0x11, 0xE0, 0x0F, 0x00, 0x11, 0x40, 0x27, 0x80, 0x0A, 0xA0, 0x0A, 0xA0
    }
};

// create the ModelGauge object with the model config
ModelGauge model_gauge(model_config_example);

void setup()
{
    Serial.begin();
    waitFor(Serial.isConnected, 5000);
    delay(50);

    // load model config when power on 
    model_gauge.load_config();
}

void loop()
{
    static uint32_t last_10s = 0;
    static uint32_t last_1h = 0;

    // print soc every 10 seconds
    if(System.uptime() - last_10s >= 10)
    {
        last_10s = System.uptime();
        
        // read battery voltage and soc from ModelGauge
        float volt = model_gauge.get_volt();
        float soc  = model_gauge.get_soc();
        Log.info(">>> volt:%.2f, soc:%.2f%%", volt, soc);
    }

    // verify model every 1 hour
    if(System.uptime() - last_1h >= 3600)
    {
        last_1h = System.uptime();

        // verify model, reload model if verify failed
        model_gauge.verify_model();
    }
}
