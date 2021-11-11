#ifndef __APP_SERVER_H__
#define __APP_SERVER_H__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServerSecure.h>

#include "SavedConfig.h"
#include "PWMController.h"
#include "PageManager.h"

class AppServer
{

public:
    AppServer(SavedConfig &config, PWMController &cotrol);
    ~AppServer() {}

    void init();
    void loop();

private:
    static const char *AUTH_REALM;
    static const char *MSG_AUTH_FAILED;

    // crypto for https
    static const uint8_t RSAkey[];
    static const uint8_t x509[];

    enum NetType
    {
        NET_EXT,
        NET_AP,
    };

    enum ServerState
    {
        STATE_SETUP_NET,
        STATE_SETUP_AP,
        STATE_HANDLE,
    };

    void _server_loop();
    bool _setup_net();
    bool _setup_ap();
    bool _setup_server();

    bool _http_authenticate();

    static void _handle_root();
    static void _handle_config();
    static void _handle_control();
    static void _handle_set_config();
    static void _handle_pwm_start();
    static void _handle_pwm_stop();

    static AppServer* _global_instance;

    SavedConfig &_config;
    PWMController &_control;

    int _net_type;
    int _server_state;

    ESP8266WebServerSecure _server;
    PageManager _page_manager;
    WiFiClient _client;
    X509List _x509;
    PrivateKey _pkey;
    
};

#endif