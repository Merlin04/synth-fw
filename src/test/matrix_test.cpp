#include "matrix_test.hpp"
#include <Arduino.h>

int rows[] = {35, 36, 37, 38, 39, 40, 41, 14, 15, 25, 24, 12, 7};

int cols[] = {32, 31, 30, 29, 28, 27, 26, 33, 34};

void matrix_test() {
    for(int i = 0; i < 13; i++) {
        pinMode(rows[i], INPUT_PULLDOWN);
    }
    for(int i = 0; i < 9; i++) {
        pinMode(cols[i], OUTPUT);
    }

    while(true) {
        for(int i = 0; i < 9; i++) {
            digitalWriteFast(cols[i], 1);
            for(int j = 0; j < 13; j++) {
                if(digitalReadFast(rows[j]) == 1) {
                    Serial.printf("row: %d, col: %d\n", j, i);
                }
            }
            digitalWriteFast(cols[i], 0);
            delayMicroseconds(5);
        }
//        delay(10);
    }
}

/*
void matrix_test_irq_callback_handler() {

}

void matrix_test_enable_interrupts() {
    for(int col : cols) {
        digitalWrite(col, 1);
    }
    for(int row : rows) {
        attachInterrupt(row, matrix_test_irq_callback_handler, CHANGE);
    }
}

void matrix_test_disable_interrupts() {

}

void matrix_test_interrupts() {

}*/