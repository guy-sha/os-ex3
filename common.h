#ifndef OS_EX3_COMMON_H
#define OS_EX3_COMMON_H

#ifdef DEBUG
#define __PRINT_DEBUG__ 1
#else
#define __PRINT_DEBUG__ 0
#endif

#define debug_print(fmt, ...) \
    do { if (__PRINT_DEBUG__) fprintf(stderr, "DEBUG:\t" fmt, ##__VA_ARGS__); } while (0)
#endif //OS_EX3_COMMON_H
