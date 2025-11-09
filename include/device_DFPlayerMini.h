#ifndef DFPLAYERMINI_H
#define DFPLAYERMINI_H

#include <stdint.h>
#include <protocol_UART.h>
#include <util/delay.h>

// DFPlayer Mini MP3 module driver over software UART
// Follows the project pattern: device_* inherits protocol_*
//
// Usage:
//   DFPlayerMini player(&PIND, &DDRD, &PORTD, PD4,    // TX pin (to DFPlayer RX)
//                       &PIND, &DDRD, &PORTD, PD5,    // RX pin (to DFPlayer TX)
//                       9600);                         // Baud rate
//   
//   sei();  // Enable interrupts for RX
//   delay(1000);  // Wait for module power-up
//   
//   if (!player.begin()) {
//     // Error: no card detected
//   }
//   
//   player.setVolume(25);  // 0-30
//   player.playTrack(1);   // Play 0001.mp3
//
class DFPlayerMini : protected UART {
public:
    DFPlayerMini(volatile uint8_t *tx_pin_reg, volatile uint8_t *tx_ddr, volatile uint8_t *tx_port, uint8_t tx_pin,
                 volatile uint8_t *rx_pin_reg, volatile uint8_t *rx_ddr, volatile uint8_t *rx_port, uint8_t rx_pin,
                 unsigned long baud = 9600, bool feedback = false)
        : UART(tx_pin_reg, tx_ddr, tx_port, tx_pin, rx_pin_reg, rx_ddr, rx_port, rx_pin, baud),
          wantFeedback(feedback) {}

    DFPlayerMini() = delete;

    // Initialize DFPlayer - gives module time to mount SD card
    // Call this in setup() to ensure module is ready before sending commands
    // Parameters:
    //   doReset: Send reset command (usually not needed, default false)
    //   timeoutMs: Delay to allow card mounting (default 2000ms = 2 seconds)
    // Returns: true (always, for API compatibility with response-checking versions)
    bool begin(bool doReset = false, uint16_t timeoutMs = 2000);

    // Basic control API
    void selectTF();
    void reset();
    void setVolume(uint8_t vol);

    // Play controls
    void playTrack(uint16_t index);
    void loopTrack(uint16_t index);
    void playFolderTrack(uint8_t folder, uint8_t index);
    void setPlaybackMode(uint8_t mode);  // 0=repeat, 1=folder repeat, 2=single repeat, 3=random
    void pause();
    void resume();
    void stop();
    void next();
    void prev();

    // Expose minimal RX to let users poll responses if feedback=true
    int available() const;
    int read();
    int peek() const;
    void flush();

    // Wait for DFPlayer to initialize (waits for "Card Online" response like official library)
    // Returns true if card detected, false on timeout
    bool waitForCardOnline(uint16_t timeoutMs = 3000);

private:
    bool wantFeedback;

    // DFPlayer command codes
    static const uint8_t CMD_NEXT        = 0x01;
    static const uint8_t CMD_PREV        = 0x02;
    static const uint8_t CMD_PLAY_IDX    = 0x03;
    static const uint8_t CMD_INC_VOL     = 0x04;
    static const uint8_t CMD_DEC_VOL     = 0x05;
    static const uint8_t CMD_SET_VOL     = 0x06;
    static const uint8_t CMD_LOOP        = 0x08;
    static const uint8_t CMD_PLAYBACK_MODE = 0x08;  // Alias: 0=repeat, 1=folder repeat, 2=single repeat, 3=random
    static const uint8_t CMD_SEL_DEV     = 0x09;
    static const uint8_t CMD_RESET       = 0x0C;
    static const uint8_t CMD_RESUME      = 0x0D;
    static const uint8_t CMD_PAUSE       = 0x0E;
    static const uint8_t CMD_PLAY_FOLDER = 0x0F;
    static const uint8_t CMD_STOP        = 0x16;

    // DFPlayer response codes (from official library)
    static const uint8_t RSP_CARD_INSERTED = 0x3A;
    static const uint8_t RSP_CARD_ONLINE   = 0x3F;  // Card ready - bit 1 for TF card
    static const uint8_t RSP_USB_ONLINE    = 0x3F;  // USB ready - bit 0 for USB

    // Parse received 10-byte response frame
    // Returns: response command byte (0x3F for Card Online), or 0 if invalid/incomplete
    uint8_t parseResponse(uint16_t &parameter);

    void sendCommand(uint8_t cmd, uint16_t param);
};

#endif // DFPLAYERMINI_H