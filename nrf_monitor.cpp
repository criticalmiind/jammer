/*
 * nrf_monitor.cpp — NRF24L01 2.4 GHz spectrum sweep
 *
 * Uses Module #1 on the VSPI bus in receive mode.  For each of
 * the 126 NRF channels (2400-2525 MHz) we briefly listen and
 * check the RPD (Received Power Detector) / CD register.
 * Multiple passes are averaged for a smoother graph.
 */

#include "nrf_monitor.h"

// VSPI instance for NRF module #1
static SPIClass _vspi(VSPI);
static RF24 _radio(NRF1_CE, NRF1_CSN);
static bool _connected = false;

#define SWEEP_PASSES 3   // Number of passes per sweep for averaging

/*
 * nrf_monitor_init()
 * Initializes the radio in receive mode with auto-ack off.
 */
bool nrf_monitor_init() {
  _vspi.begin(18, 19, 23, NRF1_CSN);    // SCK, MISO, MOSI, SS
  if (!_radio.begin(&_vspi)) {
    DBGLN(F("[NRF] Module #1 NOT detected"));
    _connected = false;
    return false;
  }
  _radio.setAutoAck(false);
  _radio.stopListening();
  _radio.setDataRate(RF24_1MBPS);
  _radio.setPALevel(RF24_PA_MIN);
  _radio.startListening();
  _connected = true;
  DBGLN(F("[NRF] Module #1 initialized (spectrum mode)"));
  return true;
}

/*
 * nrf_monitor_sweep()
 * Scans channels 0-125 and stores carrier-detect hits.
 * Higher values = more activity on that channel.
 */
void nrf_monitor_sweep(uint8_t channelData[]) {
  if (!_connected) {
    memset(channelData, 0, NRF_NUM_CHANNELS);
    return;
  }

  memset(channelData, 0, NRF_NUM_CHANNELS);

  for (uint8_t pass = 0; pass < SWEEP_PASSES; pass++) {
    for (uint8_t ch = 0; ch < NRF_NUM_CHANNELS; ch++) {
      _radio.setChannel(ch);
      _radio.startListening();
      delayMicroseconds(NRF_SWEEP_DELAY_US);
      _radio.stopListening();

      if (_radio.testCarrier()) {        // RPD: signal above -64 dBm
        if (channelData[ch] < 255) {
          channelData[ch]++;
        }
      }
    }
  }
}

/*
 * nrf_monitor_peakChannel()
 * Returns the channel with the highest activity count.
 */
uint8_t nrf_monitor_peakChannel(const uint8_t channelData[]) {
  uint8_t peak = 0;
  for (uint8_t i = 1; i < NRF_NUM_CHANNELS; i++) {
    if (channelData[i] > channelData[peak]) {
      peak = i;
    }
  }
  return peak;
}

bool nrf_monitor_isConnected() {
  return _connected;
}
