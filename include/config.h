#ifndef __CONFIG_H__
#define __CONFIG_H__

#define PIN_INPUT 0
#define PIN_OUTPUT 2

#define SERIAL_FREQ 115200

#define SAVED_CONFIG_PATH "/config.json"

#define HTML_TEXT_INPUT_MAX_LENGTH 64

#define PWM_RANGE 1024

#define PWM_MIN_FREQ 100 // timer limit
#define PWM_MAX_FREQ 1000 // 1us resolution limit

#define PWM_MIN_WIDTH 1
#define PWM_MAX_WIDTH 10000 // full range at min freq limit

#define MAX_CONTENT_SIZE 1460 // TCP buffer limit

#endif