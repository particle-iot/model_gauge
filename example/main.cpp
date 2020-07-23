
#include "Particle.h"
#include "model_gauge.h"

SerialLogHandler logHandler(115200, LOG_LEVEL_INFO);
SYSTEM_MODE(MANUAL);

// define your new battery model config first, or use the default config in model_gauge.h
// example: LG INR21700 battery model get from the .ini file
const model_config_t model_config_example = {
    .EmptyAdjustment=0,
    .FullAdjustment=100,
    .RCOMP0 = 92,
    .TempCoUp = -0.453125,
    .TempCoDown = -0.8125,
    .OCVTest = 58560,
    .SOCCheckA = 203,
    .SOCCheckB = 205,
    .bits = 19,
    .model_data = {
        0x88, 0x70, 0xAA, 0x10, 0xAD, 0x90, 0xB0, 0x60, 0xB3, 0xF0, 0xB7, 0x00, 0xB8, 0xF0, 0xBC, 0x50,
        0xBF, 0xE0, 0xC2, 0x00, 0xC4, 0x60, 0xC7, 0x40, 0xCA, 0xD0, 0xCC, 0x40, 0xCD, 0x00, 0xDA, 0xC0, 
        0x00, 0x40, 0x07, 0x00, 0x0C, 0x00, 0x10, 0x40, 0x13, 0x00, 0x1D, 0x60, 0x19, 0x20, 0x1A, 0xE0,
        0x13, 0xC0, 0x15, 0x80, 0x11, 0xC0, 0x13, 0x20, 0x3D, 0x00, 0x5E, 0x60, 0x01, 0x20, 0x01, 0x20,
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
