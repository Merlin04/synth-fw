#pragma once

#include <cstdarg>
#include <cstdio>
#include <Arduino.h>

template<bool enable_debug>
class Log {
    const char* ns;
public:
    explicit Log(const char* ns) : ns(ns) {}

    // ReSharper disable once CppMemberFunctionMayBeConst
    // no it can't be, it has side effects
    int debug(const char* format, ...) {
        if(!enable_debug) return 0;
        Serial.printf("D [%d] [%s] ", millis(), ns);
        va_list args;
        va_start(args, format);
        const int ret = vdprintf(Serial, format, args);
        va_end(args);
        return ret;
    }
};
