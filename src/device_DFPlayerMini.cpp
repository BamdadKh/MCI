#include <device_DFPlayerMini.h>
#include <util/delay.h>
#include <Arduino.h>  // For millis()

// -------- Public API Implementation --------

// Basic control commands
void DFPlayerMini::selectTF() {
    sendCommand(CMD_SEL_DEV, 0x0002);
}

void DFPlayerMini::reset() {
    sendCommand(CMD_RESET, 0x0000);
}

void DFPlayerMini::setVolume(uint8_t vol) {
    if (vol > 30) vol = 30;
    sendCommand(CMD_SET_VOL, vol);
}

// Play control commands
void DFPlayerMini::playTrack(uint16_t index) {
    if (index == 0) index = 1;
    sendCommand(CMD_PLAY_IDX, index);
}

void DFPlayerMini::loopTrack(uint16_t index) {
    if (index == 0) index = 1;
    // Working sequence from blinkLED project:
    // 1. Play the track first
    sendCommand(CMD_PLAY_IDX, index);
    // 2. Then set to loop mode (parameter 0x00, not the track number!)
    sendCommand(CMD_PLAYBACK_MODE, 0x0000);
}

void DFPlayerMini::playFolderTrack(uint8_t folder, uint8_t index) {
    uint16_t param = ((uint16_t)folder << 8) | (uint16_t)index;
    sendCommand(CMD_PLAY_FOLDER, param);
}

void DFPlayerMini::setPlaybackMode(uint8_t mode) {
    // 0 = repeat all, 1 = folder repeat, 2 = single repeat, 3 = random
    if (mode > 3) mode = 0;
    sendCommand(CMD_PLAYBACK_MODE, mode);
}

void DFPlayerMini::pause() {
    sendCommand(CMD_PAUSE, 0x0000);
}

void DFPlayerMini::resume() {
    sendCommand(CMD_RESUME, 0x0000);
}

void DFPlayerMini::stop() {
    sendCommand(CMD_STOP, 0x0000);
}

void DFPlayerMini::next() {
    sendCommand(CMD_NEXT, 0x0000);
}

void DFPlayerMini::prev() {
    sendCommand(CMD_PREV, 0x0000);
}

// RX API wrappers
int DFPlayerMini::available() const {
    return UART::available();
}

int DFPlayerMini::read() {
    return UART::read();
}

int DFPlayerMini::peek() const {
    return UART::peek();
}

void DFPlayerMini::flush() {
    UART::flush();
}

// -------- Initialization --------

// Initialize DFPlayer module - waits for "Card Online" or "USB Online" response
// Official DFRobot library behavior:
// - Optionally sends reset command
// - Waits up to timeoutMs for module to report card/USB ready
// - Returns true if storage device detected
// 
// Note: Most DFPlayer clones don't send unsolicited "Card Online" messages.
// They only respond when feedback is explicitly requested in commands.
// So we use a simple delay-based approach instead.
bool DFPlayerMini::begin(bool doReset, uint16_t timeoutMs) {
    // Flush any stale data in RX buffer
    flush();
    
    if (doReset) {
        reset();
        _delay_ms(200);  // Give reset time to execute
    }
    
    // Most DFPlayer modules don't send automatic "Card Online" messages
    // unless feedback is enabled in every command. Instead, we just
    // give the module time to mount the SD card after power-up.
    // Official modules take 1-3 seconds to be ready.
    if (timeoutMs > 0) {
        delay(timeoutMs);
    }
    
    // Assume success - the module will simply not play if card is missing
    // User can check this by whether playback actually starts
    return true;
}

// Wait for DFPlayer module to send "Card Online" response
// Official library waits up to 2 seconds after reset
bool DFPlayerMini::waitForCardOnline(uint16_t timeoutMs) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeoutMs) {
        if (available() >= 10) {
            uint16_t param;
            uint8_t cmd = parseResponse(param);
            
            // 0x3F is "Card Online" - check parameter for card type
            // bit 0 = USB, bit 1 = TF/SD card
            if (cmd == RSP_CARD_ONLINE) {
                if (param & 0x02) {  // TF card online
                    return true;
                }
                if (param & 0x01) {  // USB online
                    return true;
                }
            }
        }
        _delay_ms(10);  // Poll every 10ms
    }
    
    return false;  // Timeout
}

// -------- Private Helper Methods --------

// Parse received 10-byte response frame
// Returns: response command byte (0x3F for Card Online), or 0 if invalid/incomplete
uint8_t DFPlayerMini::parseResponse(uint16_t &parameter) {
    if (UART::available() < 10) return 0;  // Need full 10-byte frame
    
    uint8_t frame[10];
    for (int i = 0; i < 10; i++) {
        int b = UART::read();
        if (b < 0) return 0;
        frame[i] = (uint8_t)b;
    }
    
    // Validate frame structure
    if (frame[0] != 0x7E || frame[1] != 0xFF || frame[2] != 0x06 || frame[9] != 0xEF) {
        return 0;  // Invalid frame
    }
    
    // Validate checksum
    uint16_t sum = frame[1] + frame[2] + frame[3] + frame[4] + frame[5] + frame[6];
    uint16_t receivedCs = ((uint16_t)frame[7] << 8) | frame[8];
    uint16_t expectedCs = 0xFFFF - sum + 1;
    if (receivedCs != expectedCs) {
        return 0;  // Checksum mismatch
    }
    
    // Extract command and parameter
    parameter = ((uint16_t)frame[5] << 8) | frame[6];
    return frame[3];  // Return command byte
}

// Send command to DFPlayer Mini
void DFPlayerMini::sendCommand(uint8_t cmd, uint16_t param) {
    uint8_t f[10];
    f[0] = 0x7E;    // start
    f[1] = 0xFF;    // version
    f[2] = 0x06;    // length
    f[3] = cmd;     // command
    f[4] = wantFeedback ? 0x01 : 0x00; // feedback
    f[5] = (uint8_t)((param >> 8) & 0xFF);
    f[6] = (uint8_t)(param & 0xFF);
    uint16_t sum = (uint16_t)(f[1] + f[2] + f[3] + f[4] + f[5] + f[6]);
    uint16_t cs = (uint16_t)(0xFFFF - sum + 1);
    f[7] = (uint8_t)((cs >> 8) & 0xFF);
    f[8] = (uint8_t)(cs & 0xFF);
    f[9] = 0xEF;    // end
    UART::sendBytes(f, sizeof(f));
    
    // CRITICAL: DFPlayer needs time to process each command
    // Official library uses 10ms delay after each command
    _delay_ms(10);
}

// -------- Deprecated Helper Functions --------

// DEPRECATED: Old convenience function - use begin() instead
bool DFPlayerMini_initializeTF(DFPlayerMini &df, uint8_t volume) {
    // Give module time to boot, then select TF and set volume.
    _delay_ms(1500);
    df.selectTF();
    _delay_ms(120);
    df.setVolume(volume);
    _delay_ms(120);
    return true;
}
