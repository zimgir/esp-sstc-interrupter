#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "config.h"
#include "utils.h"
#include "SavedConfig.h"

const char *SavedConfig::PATH_CONFIG_FILE = SAVED_CONFIG_PATH;

const char *SavedConfig::VAL_NOT_SET = "<NOT SET>";

// network config
const char *SavedConfig::JSON_KEY_NET_SSID = "net_ssid";
const char *SavedConfig::JSON_KEY_NET_PASS = "net_pass";
const char *SavedConfig::JSON_KEY_AP_SSID = "ap_ssid";
const char *SavedConfig::JSON_KEY_AP_PASS = "ap_pass";
const char *SavedConfig::JSON_KEY_AUTH_USER = "auth_user";
const char *SavedConfig::JSON_KEY_AUTH_PASS = "auth_pass";
const char *SavedConfig::JSON_KEY_MDNS_NAME = "mdns_name";
const char *SavedConfig::JSON_KEY_STATIC_IP = "static_ip";
const char *SavedConfig::JSON_KEY_SUBNET = "subnet";
const char *SavedConfig::JSON_KEY_GATEWAY = "gateway";
const char *SavedConfig::JSON_KEY_DNS = "dns";

// pwm config
const char *SavedConfig::JSON_KEY_MAX_FREQ = "max_freq";
const char *SavedConfig::JSON_KEY_MAX_WIDTH = "max_width";
const char *SavedConfig::JSON_KEY_MAX_DUTY = "max_duty";
const char *SavedConfig::JSON_KEY_MAX_DURATION = "max_duration";

SavedConfig::SavedConfig() : _net_ssid(VAL_NOT_SET),
                             _net_pass(VAL_NOT_SET),
                             _ap_ssid("esptc"),
                             _ap_pass(""),
                             _auth_user(VAL_NOT_SET),
                             _auth_pass(VAL_NOT_SET),
                             _mdns_name(VAL_NOT_SET),
                             _static_ip(VAL_NOT_SET),
                             _subnet(VAL_NOT_SET),
                             _gateway(VAL_NOT_SET),
                             _dns(VAL_NOT_SET),
                             _max_freq(500),
                             _max_width(1000),
                             _max_duration(5000),
                             _max_duty(20)

{
}

int SavedConfig::init()
{
    int ret = 0;
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();

    LOGD("\n\nFlash real id:   %08X\n", ESP.getFlashChipId());
    LOGD("Flash real size: %u bytes\n", realSize);
    LOGD("Flash ide size : %u bytes\n", ideSize);
    LOGD("Flash ide speed: %u Hz\n", ESP.getFlashChipSpeed());
    LOGD("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT"
                                                           : ideMode == FM_DIO    ? "DIO"
                                                           : ideMode == FM_DOUT   ? "DOUT"
                                                                                  : "UNKNOWN"));

    if (ideSize != realSize)
    {
        LOGI("Flash Chip configuration wrong!\n");
    }
    else
    {
        LOGI("Flash Chip configuration OK!\n");
    }

    if (!LittleFS.begin())
    {
        LOGE("Failed to initialize LittleFS!");
        return -1;
    }

    ret = load();

    return ret;
}

int SavedConfig::load()
{
    int ret = 0;

    String json_str;

    StaticJsonDocument<1024> json_config;

    File cfg_file = LittleFS.open(PATH_CONFIG_FILE, "r");

    if (!cfg_file)
    {
        LOGE("Failed to open config file!");
        return -1;
    }

    DeserializationError json_error = deserializeJson(json_config, cfg_file);

    if (json_error)
    {
        LOGE("Failed to parse config file: %s", json_error.c_str());
        ret = -2;
        goto exit;
    }

    _net_ssid = json_config[JSON_KEY_NET_SSID].as<const char *>();
    _net_pass = json_config[JSON_KEY_NET_PASS].as<const char *>();
    _ap_ssid = json_config[JSON_KEY_AP_SSID].as<const char *>();
    _ap_pass = json_config[JSON_KEY_AP_PASS].as<const char *>();
    _auth_user = json_config[JSON_KEY_AUTH_USER].as<const char *>();
    _auth_pass = json_config[JSON_KEY_AUTH_PASS].as<const char *>();
    _mdns_name = json_config[JSON_KEY_MDNS_NAME].as<const char *>();
    _static_ip = json_config[JSON_KEY_STATIC_IP].as<const char *>();
    _subnet = json_config[JSON_KEY_SUBNET].as<const char *>();
    _gateway = json_config[JSON_KEY_GATEWAY].as<const char *>();
    _dns = json_config[JSON_KEY_DNS].as<const char *>();

    _max_freq = json_config[JSON_KEY_MAX_FREQ].as<uint32_t>();
    _max_width = json_config[JSON_KEY_MAX_WIDTH].as<uint32_t>();
    _max_duty = json_config[JSON_KEY_MAX_DUTY].as<uint32_t>();
    _max_duration = json_config[JSON_KEY_MAX_DURATION].as<uint32_t>();

    LOGI("Successfully loaded configuration! size: %u", json_config.memoryUsage());

    serializeJsonPretty(json_config, json_str);
    LOGD("\n\nLoaded config:\n\n%s\n\n", json_str.c_str());

exit:
    cfg_file.close();

    return ret;
}

int SavedConfig::save()
{
    int ret = 0;

    String json_str;

    StaticJsonDocument<1024> json_config;

    File cfg_file = LittleFS.open(PATH_CONFIG_FILE, "w");

    if (!cfg_file)
    {
        LOGE("Failed to open config file!");
        return -1;
    }

    json_config[JSON_KEY_NET_SSID] = _net_ssid;
    json_config[JSON_KEY_NET_PASS] = _net_pass;
    json_config[JSON_KEY_AP_SSID] = _ap_ssid;
    json_config[JSON_KEY_AP_PASS] = _ap_pass;
    json_config[JSON_KEY_AUTH_USER] = _auth_user;
    json_config[JSON_KEY_AUTH_PASS] = _auth_pass;
    json_config[JSON_KEY_MDNS_NAME] = _mdns_name;
    json_config[JSON_KEY_STATIC_IP] = _static_ip;
    json_config[JSON_KEY_SUBNET] = _subnet;
    json_config[JSON_KEY_GATEWAY] = _gateway;
    json_config[JSON_KEY_DNS] = _dns;

    json_config[JSON_KEY_MAX_FREQ] = _max_freq;
    json_config[JSON_KEY_MAX_WIDTH] = _max_width;
    json_config[JSON_KEY_MAX_DUTY] = _max_duty;
    json_config[JSON_KEY_MAX_DURATION] = _max_duration;

    serializeJsonPretty(json_config, json_str);
    LOGD("\n\nSaved config:\n\n%s\n\n", json_str.c_str());

    if (0 == serializeJson(json_config, cfg_file))
    {
        ret = -2;
        goto exit;
    }

    LOGI("Successfully saved configuration! size: %u", json_config.memoryUsage());

exit:
    cfg_file.close();

    return ret;
}

enum FormInterface::SetResult SavedConfig::set(const String &key, const String &val, String &msg)
{
    uint32_t parsed_int;
    float parsed_float;

    if (val.length() > HTML_TEXT_INPUT_MAX_LENGTH)
        return SET_VAL_TOO_LONG;

    switch (key.toInt())
    {
    case FORM_KEY_NET_SSID:
    {
        _net_ssid = val;

        break;
    }
    case FORM_KEY_NET_PASS:
    {
        _net_pass = val;

        break;
    }
    case FORM_KEY_AP_SSID:
    {
        _ap_ssid = val;

        break;
    }
    case FORM_KEY_AP_PASS:
    {
        _ap_pass = val;

        break;
    }
    case FORM_KEY_AUTH_USER:
    {
        _auth_user = val;

        break;
    }
    case FORM_KEY_AUTH_PASS:
    {
        _auth_pass = val;

        break;
    }
    case FORM_KEY_MDNS_NAME:
    {
        _mdns_name = val;

        break;
    }
    case FORM_KEY_STATIC_IP:
    {
        if (!_check_ip(val, msg, JSON_KEY_STATIC_IP))
            return SET_INVALID_VALUE;

        _static_ip = val;

        break;
    }
    case FORM_KEY_SUBNET:
    {
        if (!_check_ip(val, msg, JSON_KEY_SUBNET))
            return SET_INVALID_VALUE;

        _subnet = val;

        break;
    }
    case FORM_KEY_GATEWAY:
    {
        if (!_check_ip(val, msg, JSON_KEY_GATEWAY))
            return SET_INVALID_VALUE;

        _gateway = val;

        break;
    }
    case FORM_KEY_DNS:
    {
        if (!_check_ip(val, msg, JSON_KEY_DNS))
            return SET_INVALID_VALUE;

        _dns = val;

        break;
    }
    case FORM_KEY_MAX_FREQ:
    {
        parsed_int = (uint32_t)val.toInt();
        if(parsed_int > 1000000)
        {
            msg = "Max frequency can't be more than 1MHz since period can't be smaller than 1 us";
            return SET_INVALID_VALUE;
        }

        _max_freq = parsed_int;

        break;
    }
    case FORM_KEY_MAX_WIDTH:
    {
        parsed_int = (uint32_t)val.toInt();
        if(parsed_int > 1000000)
        {
            msg = "Max width can't be more than 1 second";
            return SET_INVALID_VALUE;
        }

        _max_width = parsed_int;

        break;

    }
    case FORM_KEY_MAX_DURATION:
    {
        parsed_int = (uint32_t)val.toInt();

        if(parsed_int > 3600000)
        {
            msg = "Max duration  can't be more than 1 hour";
            return SET_INVALID_VALUE;
        }

        _max_duration = parsed_int;

        break;

    }
    case FORM_KEY_MAX_DUTY:
    {
        parsed_float = val.toFloat();

        if(parsed_float > 100)
        {
            msg = "Max duty cycle can't be more than 100%";
            return SET_INVALID_VALUE;
        }

        _max_duty = parsed_float;

        break;

    }
    default:
        return SET_INVALID_KEY;
    }

    return SET_OK;
}

bool SavedConfig::_check_ip(const String &val, String &msg, const char *ipname)
{
    if (!_test_ip.fromString(val))
    {
        msg = String("Invalid IP for: ") + ipname;
        return false;
    }

    return true;
}
