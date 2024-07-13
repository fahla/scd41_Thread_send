/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h> 
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>


#include "scd4x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"

#include <stdio.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/udp.h>


/**
 * TO USE CONSOLE OUTPUT (PRINTF) IF NOT PRESENT ON YOUR PLATFORM
 */
//#define printf(...)

int main(void) {
    
     char message[100];
    int16_t error = 0;

    sensirion_i2c_hal_init();
    // Clean up potential SCD40 states
    scd4x_wake_up();
    scd4x_stop_periodic_measurement();
    scd4x_reinit();

    otInstance* myInstance;
    myInstance = openthread_get_default_instance();
    otUdpSocket mySocket;
    otMessageInfo messageInfo;
    memset(&messageInfo, 0, sizeof(messageInfo));
    otIp6AddressFromString("ff03::1", &messageInfo.mPeerAddr);
    messageInfo.mPeerPort = 5678;


    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;

    error = scd4x_get_serial_number(&serial_0, &serial_1, &serial_2);
    if (error) {
        printk("Error executing scd4x_get_serial_number(): %i\n", error);
    } else {
        printk("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
    }

    // Start Measurement

    error = scd4x_start_periodic_measurement();
    if (error) {
        printk("Error executing scd4x_start_periodic_measurement(): %i\n",
               error);
    }

    printk("Waiting for first measurement... (5 sec)\n");

    for (;;) {
        // Read Measurement
        sensirion_i2c_hal_sleep_usec(100000);
        bool data_ready_flag = false;
        error = scd4x_get_data_ready_flag(&data_ready_flag);
        if (error) {
            printk("Error executing scd4x_get_data_ready_flag(): %i\n", error);
            continue;
        }
        if (!data_ready_flag) {
            continue;
        }
        otError error = OT_ERROR_NONE;
        uint16_t co2;
        int32_t temperature;
        int32_t humidity;
        error = scd4x_read_measurement(&co2, &temperature, &humidity);
        if (error) {
            printk("Error executing scd4x_read_measurement(): %i\n", error);
        } else if (co2 == 0) {
            printk("Invalid sample detected, skipping.\n");
        } else {
            printk("CO2: %u ppm\n", co2);
            printk("Temperature: %d °C\n", temperature);
            printk("Humidity: %d percent RH\n", humidity);
            // Format the sensor data as a string
            snprintf(message, sizeof(message), "CO2: %u ppm \n Temperature: %d °C\n Humidity: %d percent RH", co2, temperature, humidity);

            

            do {
	            error = otUdpOpen(myInstance, &mySocket, NULL, NULL);
	            if (error != OT_ERROR_NONE) { break; }
	            otMessage* test_Message = otUdpNewMessage(myInstance, NULL);
	            error = otMessageAppend(test_Message, message, (uint16_t)strlen(message));
	            if (error != OT_ERROR_NONE) { break; }
	            error = otUdpSend(myInstance, &mySocket, test_Message, &messageInfo);
	            if (error != OT_ERROR_NONE) { break; }
	            error = otUdpClose(myInstance, &mySocket);
            } while (false);
            if (error == OT_ERROR_NONE) {
	            printk("Send.\n");
            }
            else {
	            printk("udpSend error: %d\n", error);
            }
        }
    }

    return 0;
}
