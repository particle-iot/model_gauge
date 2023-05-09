/*
 * Copyright (c) 2022 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "model_gauge.h"

#if MODEL_GAUGE_DEBUG
    static  Logger local_log("ModelGauge");
    #define LOGI   local_log.info
    #define LOGE   local_log.error
#else
    #define LOGI(...)
    #define LOGE(...)
#endif

constexpr int ModelGaugeSetupRetries {100};

namespace
{
    TwoWire *fuelWireInstance()
    {
#if HAL_PLATFORM_FUELGAUGE_MAX17043
        switch (HAL_PLATFORM_FUELGAUGE_MAX17043_I2C)
        {
        case HAL_I2C_INTERFACE1:
        default:
        {
            return &Wire;
        }
#if Wiring_Wire1
        case HAL_I2C_INTERFACE2:
        {
            return &Wire1;
        }
#endif // Wiring_Wire1
#if Wiring_Wire3
        case HAL_I2C_INTERFACE3:
        {
            return &Wire3;
        }
#endif // Wiring_Wire3
        }
#endif // HAL_PLATFORM_FUELGAUGE_MAX17043

        return &Wire;
    }
} // namespace

ModelGauge::ModelGauge(const model_config_t &config) : _wire(*fuelWireInstance()), _config(config)
{
}

ModelGaugeStatus ModelGauge::load_config()
{
    byte original_OCV_1, original_OCV_2;
    int retries {ModelGaugeSetupRetries};

    while(0 <= retries)
    {
        // unlock model access
        write_word(0x3E, 0x4A, 0x57);

        // read OCV
        read_word(0x0E, original_OCV_1, original_OCV_2);
        LOGI("read original OCV: %d, %d", original_OCV_1, original_OCV_2);

        // verify model access unlocked
        if((original_OCV_1 == 0xFF) && (original_OCV_2 == 0xFF))
        {
            // failed and retry
            LOGI("verify model access unlocked: failed");
            delay(100);
        }
        else
        {
            LOGI("verify model access unlocked: success");
            break;
        }
        retries--;
    }

    if (0 >= retries) {
        return ModelGaugeStatus::IO;
    }

    ModelGaugeStatus ret {ModelGaugeStatus::NONE};

    // write RCOMP to its Maximum value (MAX17040/1/3/4 only)
    write_word(0x0C, 0xFF, 0x00);

    // write the model
    WITH_LOCK(_wire)
    {
        for(int i = 0; i < 64; i += 16)
        {
            _wire.beginTransmission(MAX17043_ADDRESS);
            _wire.write(0x40+i);
            for(int k = 0; k < 16; k++)
            {
                _wire.write(_config.model_data[i+k]);
            }
            _wire.endTransmission(true);
        }
    }

    // delay at least 150ms (MAX17040/1/3/4 only)
    delay(150);

    // write OCV
    write_word(0x0E, (_config.OCVTest >> 8) & 0xFF, _config.OCVTest & 0xFF);

    // delay between 150ms and 600ms
    delay(150);

    // read SOC register and compare to expected result
    byte SOC_1, SOC_2;
    read_word(0x04, SOC_1, SOC_2);
    if(SOC_1 >= _config.SOCCheckA && SOC_1 <= _config.SOCCheckB)
    {
        LOGI("load model successfully");
    }
    else
    {
        LOGE("load model failed");
        ret = ModelGaugeStatus::IO;
    }

    // restore CONFIG and OCV
    write_word(0x0C, _config.RCOMP0, 0x00);
    write_word(0x0E, original_OCV_1, original_OCV_2);


    // lock model access
    write_word(0x3E, 0x00, 0x00);

    // delay at least 150ms
    delay(150);

    return ret;
}

float ModelGauge::get_soc()
{
    // report state of charge of the cell
    byte SOC_1, SOC_2;
    float SOC_percent = 0;
    read_word(0x04, SOC_1, SOC_2);
    if(_config.bits == 18)
    {
        SOC_percent = ((SOC_1 << 8) + SOC_2) / 256.0f;
    }
    else if(_config.bits == 19)
    {
        SOC_percent = ((SOC_1 << 8) + SOC_2) / 512.0f;
    }
    return SOC_percent;
}

float ModelGauge::get_volt()
{
    byte VOLT_1, VOLT2;
    read_word(0x02, VOLT_1, VOLT2);

    // VCELL = 12-bit value, 1.25mV (1V/800) per bit
    float value = (float)((VOLT_1 << 4) | (VOLT2 >> 4));
    return value / 800.0;
}

ModelGaugeStatus ModelGauge::verify_model()
{
    auto ret = ModelGaugeStatus::NONE;
    byte original_OCV_1, original_OCV_2;
    byte original_RCOMP_1, original_RCOMP_2;
    byte SOC_1, SOC_2;

    write_word(0x3E, 0x4A, 0x57);
    read_word(0x0C, original_RCOMP_1, original_RCOMP_2);
    read_word(0x0E, original_OCV_1, original_OCV_2);

    write_word(0x0E, (_config.OCVTest >> 8) & 0xFF, _config.OCVTest & 0xFF);
    write_word(0x0C, original_RCOMP_1, original_RCOMP_2);
    delay(150);

    read_word(0x04, SOC_1, SOC_2);
    if(SOC_1 > _config.SOCCheckA && SOC_1 <= _config.SOCCheckB)
    {
        LOGI("model verify success");
        write_word(0x0C, original_RCOMP_1, original_RCOMP_2);
        write_word(0x0E, original_OCV_1, original_OCV_2);
    }
    else
    {
        LOGI("model verify failed, reload it");
        load_config();
        ret = ModelGaugeStatus::RELOAD;
    }
    write_word(0x3E, 0x00, 0x00);

    return ret;
}

void ModelGauge::read_word(byte address, byte &msb, byte &lsb)
{
    WITH_LOCK(_wire)
    {
        _wire.beginTransmission(MAX17043_ADDRESS);
        _wire.write(address);
        _wire.endTransmission(true);

        _wire.requestFrom(MAX17043_ADDRESS, 2, true);
        msb = _wire.read();
        lsb = _wire.read();
    }
}

void ModelGauge::write_word(byte address, byte msb, byte lsb)
{
    WITH_LOCK(_wire)
    {
        _wire.beginTransmission(MAX17043_ADDRESS);
        _wire.write(address);
        _wire.write(msb);
        _wire.write(lsb);
        _wire.endTransmission(true);
    }
}
