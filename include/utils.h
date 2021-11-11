#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>

#define _STR(s) #s
#define STR(s) _STR(s)

#define LOG(msg, ...)                      \
    do                                     \
    {                                      \
        Serial.printf(msg, ##__VA_ARGS__); \
    } while (0)

#define LOGI(msg, ...) LOG("\n\n" msg "\n\n", ##__VA_ARGS__)
#define LOGE(msg, ...) LOG("\n\nERROR: %s: " msg "\n\n", __func__, ##__VA_ARGS__)
#define LOGD(msg, ...) LOG(msg, ##__VA_ARGS__)

#define BUG(condition)                                                              \
    do                                                                              \
    {                                                                               \
        if (condition)                                                              \
        {                                                                           \
            while (1)                                                               \
            {                                                                       \
                Serial.printf("BUG in %s at %s:%d ", __func__, __FILE__, __LINE__); \
                delay(1000);                                                        \
            }                                                                       \
        }                                                                           \
    } while (0)

#define RAW_REG(addr) (volatile uint32_t *)(addr)

#if 0 // no static_sssert
#define _COMPILER_ASSERT_3(expr,msg) typedef char msg[(!!(expr))*2-1]
#define _COMPILER_ASSERT_2(expr,line) _COMPILER_ASSERT_3(expr,assert_failed_on_line_##line)
#define _COMPILER_ASSERT_1(expr,line) _COMPILER_ASSERT_2(expr,line)
#define COMPILER_ASSERT(expr, msg) _COMPILER_ASSERT_1(expr,__LINE__)
#else
#define COMPILER_ASSERT(expr, msg) static_assert((expr),msg)
#endif



#endif