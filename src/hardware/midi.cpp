#include "midi.hpp"
#include "usb_names.h"
#include <TeensyThreads.h>
#define MIDI_INTERFACE 2 // so ide knows midi is enabled
#include <usb_midi.h>
#include <kscan/kscan_gpio_matrix.hpp>
#include <util/log.hpp>

namespace Midi {
    constexpr uint8_t channel = 1;
    auto l = new Log<true>("midi");

    void init() {
        // keep midi send buffer empty
        threads.addThread([] {
            while(true) {
                usbMIDI.read();
                threads.delay(10);
            }
        });
    }

    // send high res velocity (14 bits) with the note
    // we're only using 8 bits so this accepts a uint8_t
    void send_note_on(const uint8_t note, const uint8_t velocity) {
        // get the last bit and make it the 7th
        const uint8_t lsb = (velocity & 1) << 7;
        usbMIDI.sendControlChange(0x58, lsb, channel);
        // get upper 7 bits
        const uint8_t msb = velocity >> 1;
        usbMIDI.sendNoteOn(note, msb, channel);
        l->debug("midi_note_send_on: %d %d %d\n", note, msb, lsb);
    }

    void send_note_off(const uint8_t note) {
        // ideally we'd be using running status but
        // the teensy midi library doesn't support it
        usbMIDI.sendNoteOff(note, 0, channel);
        l->debug("midi_note_send_off: %d\n", note);
    }

    void velocity_handler(const uint8_t row, const uint8_t column, const uint8_t velocity, const bool pressed) {
        if(pressed) {
            send_note_on(row * COLS_LEN + column, velocity);
        } else {
            send_note_off(row * COLS_LEN + column);
        }
    }
}

extern "C" {
    usb_string_descriptor_struct usb_string_product_name = {
        2 + 10 * 2,
        3,
        { 'w', 'e', 'i', 'r', 'd', '-', 'm', 'i', 'd', 'i' }
    };
    usb_string_descriptor_struct usb_string_manufacturer_name = {
        2 + 2 * 2,
        3,
        { 'm', 'e' }
    };
}