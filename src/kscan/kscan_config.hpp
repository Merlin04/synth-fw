#pragma once

// https://zmk.dev/docs/features/debouncing
// instant activate
#define INST_DEBOUNCE_PRESS_MS 0
#define INST_DEBOUNCE_RELEASE_MS 1
#define INST_DEBOUNCE_SCAN_PERIOD_MS 1
#define INST_POLL_PERIOD_MS 10

#define INST_ROWS_LEN 7 // 1 for direct
#define INST_COLS_LEN 9

// matrix

#define INST_DIODE_DIR KSCAN_ROW2COL // leave as row2col for direct