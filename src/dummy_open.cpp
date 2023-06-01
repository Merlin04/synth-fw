// idk exactly what this is chatgpt told me to do it

extern "C" int __attribute__((weak)) _open(const char* pathname, int flags, ...) {
    return -1;
}