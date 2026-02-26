/*
 * nrf_monitor.h — NRF24L01 2.4 GHz spectrum monitor
 */

#ifndef NRF_MONITOR_H
#define NRF_MONITOR_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "config.h"

// Initialize NRF24 module #1 (VSPI) for spectrum scanning
bool nrf_monitor_init();

// Perform a single sweep across all 126 channels.
// channelData[] must be at least NRF_NUM_CHANNELS elements.
// Each element contains a carrier-detect count (0-255).
void nrf_monitor_sweep(uint8_t channelData[]);

// Get the peak channel from a sweep dataset
uint8_t nrf_monitor_peakChannel(const uint8_t channelData[]);

// Check if the NRF module is connected and responding
bool nrf_monitor_isConnected();

#endif // NRF_MONITOR_H
