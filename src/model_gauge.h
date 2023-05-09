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

#pragma once

#include "Particle.h"
#include "spark_wiring_fuel.h"

#define MODEL_GAUGE_DEBUG   1

enum class ModelGaugeStatus
{
    NONE,                     /**< Success, no errors */
    RELOAD,                   /**< Model verify failed and reloaded model */
    IO,                       /**< IO error */
};

typedef struct
{
    uint8_t EmptyAdjustment;  //
    uint8_t FullAdjustment;   //
    uint8_t RCOMP0;           // Starting RCOMP value
    float TempCoUp;           // Temperature (hot) coeffiecient for RCOMP. Used in UPDATE RCOMP step
    float TempCoDown;         // Temperature (cold) coeffiecient for RCOMP. Used in UPDATE RCOMP step.
    uint32_t OCVTest;         // OCV Test value in decimal. Used in step 7
    uint8_t SOCCheckA;        // SOCCheck low value. Used to verify model
    uint8_t SOCCheckB;        // SOCCheck high value. Used to verify model
    uint8_t bits;             // 18 or 19 bit model. See Calculating SOC for details.
    // uint8_t evkit_data1[32];  // 32 bytes used for EVKit software only. Discard this data
    uint8_t model_data[64];   // 64 bytes of Model Data begin here. Write these bytes in this order to the first table address at 0x40h.
    // uint8_t evkit_data2[32];  // 32 bytes used for EVKit software only. Discard this data
} model_config_t;

// LG 21700 battery model get from the .ini file
const model_config_t model_config_lg21700 = {
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

// TODO: add the new default battery mode data here, you can also define in user app and input from load_config() function


class ModelGauge
{
public:
    /**
     * @brief ModelGauge
     * @param[in] config : custom config struct, input this data when using BAT_MODEL_CUSTOM model type
     */
    ModelGauge(const model_config_t &config=model_config_lg21700);

    /**
     * @brief load battery model config to fuel gauge by the input model
     * @retval ModelGaugeStatus::NONE Success
     * @retval ModelGaugeStatus::IO Errors ocurred during model load
     */
    ModelGaugeStatus load_config();

    /**
     * @brief read register and calculate to soc percentage
     * @return soc percentage value
     */
    float get_soc();

    /**
     * @brief read register and calculate to battery voltage
     * @return voltage value
     */
    float get_volt();

    /**
     * @brief verify the custom model in RAM
     * @details ModelGauge devices store the custom model parameters in RAM. The RAM data can be
     *  corrupted in the event of a power loss, brown out or ESD event. It is good practice to
     *  occasionally verify the model and reload if necessary. Maxim recommends doing this once per
     *  hour while the application is active. The following example shows how to verify the model.
     *  Alternatively the model can simply be reloaded once per hour without verification.
     * @retval ModelGaugeStatus::NONE Success
     * @retval ModelGaugeStatus::RELOAD Model corrupted and reloaded
     */
    ModelGaugeStatus verify_model();

protected:
    // register operation functions
    void read_word(byte address, byte &MSB, byte &LSB);
    void write_word(byte address, byte MSB, byte LSB);

private:
    TwoWire& _wire;
    const model_config_t &_config;
};
