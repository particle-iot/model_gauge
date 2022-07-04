/** 
 * @file model_gauge.h
 * @author Douglas Xie (Customer Success, Particle)
 * @version 1.0
 * @date 07/21/2020
 *
 * @brief MAX17043 FuelGauge Custom Model Configuration
 *
 * @copyright Copyright (c) 2020 Particle Industries, Inc.
 */

#pragma once

#ifndef __MODEL_GAUGE_H
#define __MODEL_GAUGE_H

#include "Particle.h"
#include "spark_wiring_fuel.h"

#define MODEL_GAUGE_DEBUG   1

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


// LG 18650 battery model get from the .ini file
const model_config_t model_config_lg18650 = {
    .EmptyAdjustment=0,
    .FullAdjustment=100,
    .RCOMP0 = 75,
    .TempCoUp = -0.625,
    .TempCoDown = -1.5,
    .OCVTest = 55568,
    .SOCCheckA = 259,
    .SOCCheckB = 261,
    .bits = 19,
    .model_data = {
        0x90, 0x80, 0x99, 0x20, 0xA6, 0xB0, 0xA9, 0x20, 0xAC, 0x80, 0xB0, 0x80, 0xB2, 0xE0, 0xB4, 0x30, 
        0xB5, 0xD0, 0xB7, 0x90, 0xBA, 0xE0, 0xBF, 0x20, 0xC2, 0x80, 0xC6, 0x10, 0xCA, 0x00, 0xCF, 0x10, 
        0x01, 0x80, 0x02, 0x60, 0x10, 0xC0, 0x0B, 0xA0, 0x0D, 0xC0, 0x17, 0x80, 0x23, 0xC0, 0x1D, 0x20, 
        0x1F, 0x80, 0x0F, 0xE0, 0x0B, 0xA0, 0x13, 0x60, 0x12, 0x60, 0x0E, 0xC0, 0x18, 0x40, 0x18, 0x40
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
     */
    void load_config();

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
     */
    void verify_model();

protected:
    // register operation functions
    void read_word(byte address, byte &MSB, byte &LSB);
    void write_word(byte address, byte MSB, byte LSB);

private:
    TwoWire& _wire;
    const model_config_t &_config;
};

#endif
