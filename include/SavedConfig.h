#ifndef __SAVED_CONFIG_H__
#define __SAVED_CONFIG_H__

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>

#include "FormInterface.h"

class SavedConfig : public FormInterface
{
public:
    enum FormKey
    {
        FORM_KEY_NET_SSID = 100,
        FORM_KEY_NET_PASS,
        FORM_KEY_AP_SSID,
        FORM_KEY_AP_PASS,
        FORM_KEY_AUTH_USER,
        FORM_KEY_AUTH_PASS,
        FORM_KEY_MDNS_NAME,
        FORM_KEY_STATIC_IP,
        FORM_KEY_SUBNET,
        FORM_KEY_GATEWAY,
        FORM_KEY_DNS,
        FORM_KEY_MAX_FREQ,
        FORM_KEY_MAX_WIDTH,
        FORM_KEY_MAX_DUTY,
        FORM_KEY_MAX_DURATION,
    };

    SavedConfig();
    ~SavedConfig() {}

    int init();
    int load();
    int save();

    enum FormInterface::SetResult set(const String &key, const String &val, String &msg) override;

    const String &net_ssid() const { return _net_ssid; }
    const String &net_pass() const { return _net_pass; }
    const String &ap_ssid() const { return _ap_ssid; }
    const String &ap_pass() const { return _ap_pass; }
    const String &auth_user() const { return _auth_user; }
    const String &auth_pass() const { return _auth_pass; }
    const String &mdns_name() const { return _mdns_name; }
    const String &static_ip() const { return _static_ip; }
    const String &subnet() const { return _subnet; }
    const String &gateway() const { return _gateway; }
    const String &dns() const { return _dns; }
    const uint32_t &max_freq() const { return _max_freq; }
    const uint32_t &max_width() const { return _max_width; }
    const uint32_t &max_duration() const { return _max_duration; }
    const float &max_duty() const {return _max_duty; }


private:

    bool _check_ip(const String &val, String &msg, const char *ipname);

    static const char *PATH_CONFIG_FILE;
    static const char *VAL_NOT_SET;

    // network config keys
    static const char *JSON_KEY_NET_SSID;
    static const char *JSON_KEY_NET_PASS;
    static const char *JSON_KEY_AP_SSID;
    static const char *JSON_KEY_AP_PASS;
    static const char *JSON_KEY_AUTH_USER;
    static const char *JSON_KEY_AUTH_PASS;
    static const char *JSON_KEY_MDNS_NAME;
    static const char *JSON_KEY_STATIC_IP;
    static const char *JSON_KEY_SUBNET;
    static const char *JSON_KEY_GATEWAY;
    static const char *JSON_KEY_DNS;

    // pwm config keys
    static const char *JSON_KEY_MAX_FREQ;
    static const char *JSON_KEY_MAX_WIDTH;
    static const char *JSON_KEY_MAX_DUTY;
    static const char *JSON_KEY_MAX_DURATION;

    IPAddress _test_ip;

    // instace config
    String _net_ssid;
    String _net_pass;
    String _ap_ssid;
    String _ap_pass;
    String _auth_user;
    String _auth_pass;
    String _mdns_name;
    String _static_ip;
    String _subnet;
    String _gateway;
    String _dns;

    uint32_t _max_freq;
    uint32_t _max_width;
    uint32_t _max_duration;
    float _max_duty;

};

#endif