/*
 * packet_injection.h — NRF24L01 2.4 GHz jamming & packet injection
 *
 * ⚠ EDUCATIONAL USE ONLY — Controlled lab environments.
 */

#ifndef PACKET_INJECTION_H
#define PACKET_INJECTION_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "config.h"

// NRF jam state
enum NrfJamState {
  NJAM_IDLE = 0,
  NJAM_CONFIRMING,
  NJAM_RUNNING,
  NJAM_STOPPED
};

// Jam mode — what type of interference to produce
enum NrfJamMode {
  NJAM_MODE_CONSTANT,    // Transmit constant carrier on one channel
  NJAM_MODE_HOPPING,     // Rapidly hop channels while transmitting
  NJAM_MODE_SWEEP        // Sweep all channels sequentially
};

// Initialize NRF modules #2 and #3 for jamming/injection
bool packet_injection_init();

// Start NRF jamming on a given channel range
// startCh and endCh are 0-125 (2400+ch MHz)
void packet_injection_jam_start(uint8_t startCh, uint8_t endCh, NrfJamMode mode);

// Stop jamming
void packet_injection_jam_stop();

// Non-blocking update — call every loop()
void packet_injection_update();

// Get current state
NrfJamState packet_injection_getState();

// Get packets transmitted count
uint32_t packet_injection_getTxCount();

// Inject a custom payload on a specific channel and address
// addr = 5-byte address, payload = data, len = payload length
bool packet_injection_send(uint8_t channel, const uint8_t addr[5],
                           const uint8_t* payload, uint8_t len);

#endif // PACKET_INJECTION_H
