#ifndef __PAGE_MANAGER_H__
#define __PAGE_MANAGER_H__

#include <Arduino.h>

#include "SavedConfig.h"
#include "PWMController.h"

#include "config.h"

#define HREF_ROOT "/"
#define HREF_CONFIG "/config"
#define HREF_CONTROL "/control"
#define HREF_SET_CONFIG "/setcfg"
#define HREF_PWM_STOP "/pwmstop"
#define HREF_PWM_START "/pwmstart"


class PopMessage
{

public:

    enum MessageType
    {
        MSG_INFO,
        MSG_WARNING,
        MSG_ERROR
    };

    PopMessage():
    str(""), type(MSG_INFO)
    {}
    PopMessage(MessageType _type, const String &_str):
    str(_str), type(MSG_INFO)
    {}
    ~PopMessage() {}

    void set(MessageType _type, const String &_str)
    {
        str = _str;
        type = _type;
    }

    String str;
    MessageType type;
};

class PageManager
{

public:

    PageManager(const SavedConfig &config, const PWMController &control, ESP8266WebServerSecure &server);
    ~PageManager() {}

    void send_root_page();
    void send_control_page();
    void send_config_page();

    void send_response(const PopMessage& msg);

private:

    void _start_chunked_page(const String &title);
    void _end_chunked_page();

    const SavedConfig &_config;
    const PWMController &_control;
    ESP8266WebServerSecure &_server;

    uint32_t _service_count;

};

#endif