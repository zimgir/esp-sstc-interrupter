#include <Arduino.h>

#include "config.h"
#include "utils.h"
#include "http.h"

#include "AppServer.h"

AppServer *AppServer::_global_instance;

const char *AppServer::AUTH_REALM = "esptc";
const char *AppServer::MSG_AUTH_FAILED = "Authentication failed!";

const uint8_t AppServer::RSAkey[] ICACHE_RODATA_ATTR = {
#include "key.h"
};

const uint8_t AppServer::x509[] ICACHE_RODATA_ATTR = {
#include "x509.h"
};

AppServer::AppServer(SavedConfig &config, PWMController &control) : _config(config),
                                                                    _control(control),
                                                                    _net_type(NET_EXT),
                                                                    _server_state(STATE_SETUP_NET),
                                                                    _server(443), // 443 is the standard HTTPS port
                                                                    _page_manager(config, control, _server),
                                                                    _x509(x509, sizeof(x509)),
                                                                    _pkey(RSAkey, sizeof(RSAkey))

{
    // dirty but simple hack for callbacks
    // make sure global instance is created only once (crude singleton)
    BUG(_global_instance != NULL);
    _global_instance = this;
}

void AppServer::init()
{
    _net_type = NET_EXT;
    _server_state = STATE_SETUP_NET;
}

void AppServer::loop()
{
    switch (_net_type)
    {
    case NET_EXT:
    {
        if (digitalRead(PIN_INPUT) == LOW)
        {
            _net_type = NET_AP;
            _server_state = STATE_SETUP_AP;
        }
        else if (WiFi.status() != WL_CONNECTED)
        {
            _server_state = STATE_SETUP_NET;
        }

        break;
    }
    case NET_AP:
    default:
        break;
    }

    _server_loop();
}

void AppServer::_server_loop()
{
    switch (_server_state)
    {
    case STATE_SETUP_NET:
    {
        _control.stop(); // turn off output while connecting

        if (_setup_net() && _setup_server())
        {
            LOGI("NET Server started at:\n\nhttps://%s:443\n\nhttps://%s.local",
                 WiFi.localIP().toString().c_str(), _config.mdns_name().c_str());
            _server_state = STATE_HANDLE;
        }

        break;
    }
    case STATE_SETUP_AP:
    {
        _control.stop(); // turn off output while connecting

        if (_setup_ap() && _setup_server())
        {
            // default AP IP is 192.168.4.1
            LOGI("AP Server started at:\n\nhttps://%s:443\n\nhttps://%s.local",
                 WiFi.localIP().toString().c_str(), _config.mdns_name().c_str());
            _server_state = STATE_HANDLE;
        }

        break;
    }
    case STATE_HANDLE:
    {
        _server.handleClient();

        break;
    }
    }
}

bool AppServer::_setup_net()
{
    LOGI("Connecting to network: %s ...", _config.net_ssid().c_str());

    // disable ap when connecting to network
    WiFi.softAPdisconnect();
    WiFi.enableAP(false);
    WiFi.begin(_config.net_ssid(), _config.net_pass());

    int count = 0;

    while (WiFi.status() != WL_CONNECTED)
    {

        delay(500);
        LOGD(".");
        count++;

        if (count > 32)
        {
            LOGE("Connection failed!");
            return false;
        }
    }

    IPAddress ip;
    IPAddress subnet;
    IPAddress gateway;
    IPAddress dns;

    if (ip.fromString(_config.static_ip()) &&
        subnet.fromString(_config.subnet()) &&
        gateway.fromString(_config.gateway()) &&
        dns.fromString(_config.dns()))
    {
        WiFi.config(ip, gateway, subnet, dns);
    }
    else
    {
        LOGE("COM network configuration failed!");
        return false;
    }

    LOGI("WiFi connected!\n"
         "\nIP address: %s"
         "\nMAC address: %s"
         "\nSubnet Mask: %s"
         "\nGateway IP: %s"
         "\nDNS IP: %s",
         WiFi.localIP().toString().c_str(),
         WiFi.macAddress().c_str(),
         WiFi.subnetMask().toString().c_str(),
         WiFi.gatewayIP().toString().c_str(),
         WiFi.dnsIP().toString().c_str());

    return true;
}

bool AppServer::_setup_ap()
{
    LOGI("Configuring access point ...");

    // disconnect from network when configuring access point
    WiFi.disconnect();
    WiFi.enableAP(true);

    int count = 0;

    while (!WiFi.softAP(_config.ap_ssid(), _config.ap_pass()))
    {
        delay(500);
        LOGD(".");
        count++;

        if (count > 32)
        {
            LOGE("Access point configuration failed!");
            return false;
        }
    }

    return true;
}

bool AppServer::_setup_server()
{
    // optional domain name
    if (MDNS.begin(_config.mdns_name()))
    {
        LOGI("MDNS responder started on domain: %s", _config.mdns_name().c_str());
    }

    _server.stop();
    _server.getServer().setRSACert(&_x509, &_pkey);
    _server.on(HREF_ROOT, HTTP_GET, _handle_root);
    _server.on(HREF_CONFIG, HTTP_GET, _handle_config);
    _server.on(HREF_CONTROL, HTTP_GET, _handle_control);
    _server.on(HREF_SET_CONFIG, HTTP_POST, _handle_set_config);
    _server.on(HREF_PWM_STOP, HTTP_GET, _handle_pwm_stop);
    _server.on(HREF_PWM_START, HTTP_POST, _handle_pwm_start);
    _server.begin();

    return true;
}

bool AppServer::_http_authenticate()
{
    // free access is granted in AP mode which is enabled physically
    if (_global_instance->_net_type == NET_AP)
        return true;

    if (!_global_instance->_server.authenticate(_global_instance->_config.auth_user().c_str(), _global_instance->_config.auth_pass().c_str()))
    {
        _server.requestAuthentication(DIGEST_AUTH, AUTH_REALM, MSG_AUTH_FAILED);
        return false;
    }

    return true;
}

void AppServer::_handle_root()
{
    PopMessage msg;

    LOGI("[REQ] %s", HREF_ROOT);

    if (!_global_instance->_http_authenticate())
        return;

    _global_instance->_page_manager.send_root_page();
}

void AppServer::_handle_config()
{
    PopMessage msg;

    LOGI("[REQ] %s", HREF_CONFIG);

    if (!_global_instance->_http_authenticate())
        return;

    _global_instance->_page_manager.send_config_page();
}

void AppServer::_handle_control()
{
    PopMessage msg;

    LOGI("[REQ] %s", HREF_CONTROL);

    if (!_global_instance->_http_authenticate())
        return;

    _global_instance->_page_manager.send_control_page();
}

void AppServer::_handle_set_config()
{
    int ret;
    int num_args;
    String key;
    String val;
    String res;
    PopMessage msg;

    LOGI("[REQ] %s", HREF_SET_CONFIG);

    if (!_global_instance->_http_authenticate())
        return;

    num_args = _global_instance->_server.args();

    for (int i = 0; i < num_args; i++)
    {
        key = _global_instance->_server.argName(i);
        val = _global_instance->_server.arg(i);

        if(key.compareTo("plain") == 0)
            continue; // skip full url arg in POST url encoded data

        ret = _global_instance->_config.set(key, val, res);

        if (ret != FormInterface::SET_OK)
        {
            if (res.length() == 0)
            {
                res = ("Failed to set key: " + key + "to value: " + val);
            }

            msg.set(PopMessage::MSG_ERROR, res);

            goto exit;
        }
    }

    ret = _global_instance->_config.save();

    if (ret)
    {
        msg.set(PopMessage::MSG_ERROR, ("Failed to save configuration! ret: " + String(ret)));
    }
    else
    {
        msg.set(PopMessage::MSG_INFO, "Successfully saved configuration");
    }

exit:
    _global_instance->_page_manager.send_response(msg);
}

void AppServer::_handle_pwm_start()
{
    int ret;
    int num_args;
    String key;
    String val;
    String res;
    PopMessage msg;

    LOGI("[REQ] %s", HREF_PWM_START);

    if (!_global_instance->_http_authenticate())
        return;

    num_args = _global_instance->_server.args();

    for (int i = 0; i < num_args; i++)
    {
        key = _global_instance->_server.argName(i);
        val = _global_instance->_server.arg(i);

        if(key.compareTo("plain") == 0)
            continue; // skip full url arg in POST url encoded data

        ret = _global_instance->_control.set(key, val, res);

        if (ret != FormInterface::SET_OK)
        {
            if (res.length() == 0)
            {
                res = ("Failed to set key: " + key + "to value: " + val);
            }

            msg.set(PopMessage::MSG_ERROR, res);

            goto exit;
        }
    }

    ret = _global_instance->_control.start();

    switch (ret)
    {
    case PWMController::START_PWM:
        msg.set(PopMessage::MSG_INFO, "Interrupter started in PWM mode");
        break;
    case PWMController::START_PWM_CLIPPED:
        msg.set(PopMessage::MSG_WARNING, "Interrupter started in PWM mode with clipped parameters! Limits: " + _global_instance->_control.limits_str());
        break;
    case PWMController::START_CW:
        msg.set(PopMessage::MSG_WARNING, "Interrupter started in CW mode - Watch for overheating");
        break;
    case PWMController::START_OFF:
        msg.set(PopMessage::MSG_ERROR, "Interrupter stopped with low parameters");
        break;
    default:
        msg.set(PopMessage::MSG_ERROR, ("Interrupter start failed! code: " + String(ret)));
        break;
    }

exit:
    _global_instance->_page_manager.send_response(msg);
}

void AppServer::_handle_pwm_stop()
{
    PopMessage msg(PopMessage::MSG_INFO, "Interrupter stopped with stop button");

    LOGI("[REQ] %s", HREF_PWM_STOP);

    if (!_global_instance->_http_authenticate())
        return;

    _global_instance->_control.stop();

    _global_instance->_page_manager.send_response(msg);
}