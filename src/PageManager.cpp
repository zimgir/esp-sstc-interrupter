#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServerSecure.h>

#include "config.h"
#include "utils.h"
#include "http.h"

#include "PageManager.h"

const char *CONTENT_TYPE_HTML = "text/html";
const char *CONTENT_TYPE_JSON = "application/json";

#define _FORM_INPUT_TEXT(label, key, value, maxlen, extra) \
    ("<label><h4>" label ":</h4><input"                    \
     " type=\"text\""                                      \
     " maxlength="                                         \
     "\"" maxlen "\""                                      \
     " name="                                              \
     "\"" +                                                \
     key +                                                 \
     "\""                                                  \
     " value="                                             \
     "\"" +                                                \
     value +                                               \
     "\""                                                  \
     " " extra                                             \
     "></label><br>\n")

#define FORM_INPUT_TEXT(label, key, value) \
    _FORM_INPUT_TEXT(label, key, value, STR(HTML_TEXT_INPUT_MAX_LENGTH), "")

// xxx.xxx.xxx.xxx (maxlen = 15)
#define FORM_INPUT_TEXT_IP(label, key, value) \
    _FORM_INPUT_TEXT(label, key, value, STR(15), "")

#define FORM_INPUT_TEXT_DISABLED(label, key, value) \
    _FORM_INPUT_TEXT(label, key, value, STR(15), "disabled")

#define _FORM_INPUT_NUMBER(label, key, value, min, max, unit, oninput) \
    ("<label><h4>" label ":</h4><input"                                \
     " type=\"number\""                                                \
     " oninput="                                                       \
     "\"" oninput "\""                                                 \
     " min="                                                           \
     "\"" STR(min) "\""                                                \
                   " max="                                             \
                   "\"" STR(max) "\""                                  \
                                 " name="                              \
                                 "\"" +                                \
     key +                                                             \
     "\""                                                              \
     " value="                                                         \
     "\"" +                                                            \
     value +                                                           \
     "\""                                                              \
     ">&nbsp;[" unit "]</label><br>\n")

#define FORM_INPUT_NUMBER(label, key, value, min, max, unit) \
    _FORM_INPUT_NUMBER(label, key, value, min, max, unit, "")

#define FORM_INPUT_NUMBER_INT(label, key, value, min, max, unit) \
    _FORM_INPUT_NUMBER(label, key, value, min, max, unit, "this.value=Math.round(this.value)")

#define FORM_INPUT_RANGE(label, id, span, key, value, min, max, step, unit) \
    ("<label><h4>" label ":</h4><input"                                     \
     " type=\"range\""                                                      \
     " class=\"slider\""                                                    \
     " id="                                                                 \
     "\"" id "\""                                                           \
     " min="                                                                \
     "\"" STR(min) "\""                                                     \
                   " max="                                                  \
                   "\"" +                                                   \
     max + "\""                                                             \
           " step="                                                         \
           "\"" STR(step) "\""                                              \
                          " name="                                          \
                          "\"" +                                            \
     key +                                                                  \
     "\""                                                                   \
     " value="                                                              \
     "\"" +                                                                 \
     value +                                                                \
     "\""                                                                   \
     "><span id="                                                           \
     "\"" span "\""                                                         \
     "></span>"                                                             \
     "&nbsp;[" unit "]</label><br>\n")

PageManager::PageManager(const SavedConfig &config,
                         const PWMController &control,
                         ESP8266WebServerSecure &server) : _config(config),
                                                           _control(control),
                                                           _server(server),
                                                           _service_count(0)

{
}

void PageManager::_start_chunked_page(const String &title)
{
    // page has to be sent in chunks to fit into available TCP buffer of MAX_CONTENT_SIZE bytes
    // large static html has to be placed in flash with PROGMEM to have enough RAM for proper operation
    // const static html from PROGMEM need to be sent with _P version of the server's send functions
    static const char header_part_1[] PROGMEM =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<style>\n"
        "body{font-family:Arial;color:white;background-color:#303636;}\n"
        "button{border-radius:6px;border:none;font-size:16px;cursor:pointer;0;padding:16px;color:white;}\n"
        "input[type=text],input[type=number],select{display:inline-block;width:30%;padding:6px;margin-right:16px;border:none;border-radius:4px;font-size:16px;color:white;background-color:#242929;}\n"
        ".gbtn{background-color:#006600;}\n"
        ".gbtn:hover{background-color:#009900;}\n"
        ".rbtn{background-color:#800000;}\n"
        ".rbtn:hover{background-color:#AA0000;}\n"
        ".hsplit{position:fixed;left:0;width:100%;}\n"
        ".menu{top:0;height:55px;background-color:black;}\n"
        ".menu button{display:inline-block;position:relative;top:2px;left:2px;width:128px;}\n";
    static const char header_part_2[] PROGMEM =
        ".title{top:55px;height:55px;text-align:center;background-color:#008000;}\n"
        ".main{top:110px;left:5%;width:90%;height:70%;overflow:auto;}\n"
        ".main label{display: block; width:90%;font-size:16px;font-weight:bold;}\n"
        ".main h4{display:inline-block;width:30%;}\n"
        ".hidden{display:none;}\n"
        ".pop{width:96%;margin:16px 0;padding:16px;border-radius:6px;}\n"
        ".pop .close{float:right;font-size:24px;margin-left:16px;line-height:16px;cursor:pointer;}\n"
        ".pop .msg{font-weight:bold}\n"
        ".slider{width:30%;position:relative;top:8px;margin-right:16px;}\n"
        ".submenu{width:70%;height:55px;}\n"
        ".submenu button{position:relative;left:30%;width:196px}\n"
        ".loader{position:fixed;top:45%;left:45%;width:24px;height:24px;z-index:2;border:12px solid black;border-radius: 50%;border-top:12px solid #009900;animation:spin 0.5s linear infinite;}\n"
        "@keyframes spin{0%{transform: rotate(0deg);}100%{transform: rotate(360deg);}}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n\n";

    ++_service_count;

    String content = (
        /* menu */
        "<div class=\"hsplit menu\">\n"
        "<button class=\"gbtn\" onclick=\"location.href='" HREF_CONFIG "';\">Configuration</button>\n"
        "<button class=\"gbtn\" onclick=\"location.href='" HREF_CONTROL "';\">Control</button>\n"
        "<button class=\"rbtn\" onclick=\"ajaxget('" HREF_PWM_STOP "');\">Stop</button>\n"
        "</div>\n\n"
        /* title */
        "<div class=\"hsplit title\">\n"
        "<h3>" +
        String(title) +
        "</h3>\n"
        "</div>\n\n"
        /* main */
        "<div class=\"hsplit main\" id=\"mdiv\">\n\n"
        "<p><span id=\"info\">Service#:&nbsp;" +
        String(_service_count) +
        "</span></p>\n"
        "<hr>\n\n"
        /* pop message */
        "<div class=\"hidden pop\" id=\"popdiv\" style=\"background-color:#009900\">\n"
        "<span class=\"msg\" id=\"poptxt\"></span>\n"
        "<span class=\"close\" onclick=\"this.parentElement.classList.add('hidden');\">x</span>\n"
        "</div>\n\n"
        /* load spinner */
        "<div class=\"hidden loader\" id=\"loader\"></div>\n\n");

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);

    COMPILER_ASSERT(sizeof(header_part_1) < MAX_CONTENT_SIZE, "header_part_1 is bigger than max content size");
    _server.send_P(HTTP_OK, CONTENT_TYPE_HTML, header_part_1);

    COMPILER_ASSERT(sizeof(header_part_2) < MAX_CONTENT_SIZE, "header_part_2 is bigger than max content size");
    _server.sendContent_P(header_part_2);

    BUG(content.length() > MAX_CONTENT_SIZE);
    _server.sendContent(content);
}

void PageManager::_end_chunked_page()
{
    static const char footer_part_1[] PROGMEM =
        "</div>\n\n" /* close main div*/
        "<script>\n"
        "var loader=document.getElementById(\"loader\");\n"
        "var popdiv=document.getElementById(\"popdiv\");\n"
        "var poptxt=document.getElementById(\"poptxt\");\n"
        "var mdiv=document.getElementById(\"mdiv\");\n"
        "var info=document.getElementById(\"info\");\n"
        "function mtop(){mdiv.scrollTo({top: 0, behavior: 'smooth'});}\n"
        "function btnsdis(dis){let btns=document.getElementsByTagName(\"button\"); for(var i = 0; i < btns.length; i++) btns[i].disabled=dis;}\n"
        "function setvis(cls,vis){if(vis) cls.classList.remove(\"hidden\"); else cls.classList.add(\"hidden\");}\n"
        "function setpop(msg,color){poptxt.innerHTML=msg;popdiv.style.background=color;setvis(popdiv,true);}\n"
        "function setinfo(txt){info.innerHTML=txt;}\n"
        "function formstr(form){let data = new FormData(form); let url = new URLSearchParams(data); return url.toString();}\n";

    static const char footer_part_2[] PROGMEM =
        "function jsonres(txt){var res; try{res = JSON.parse(txt);}catch(e){setpop('Failed to parse response!','#AA0000');setinfo(txt); return null;} return res;}\n"
        "function ajaxnew(){var xhr = new XMLHttpRequest(); xhr.timeout=10000; xhr.onreadystatechange=ajaxrdy; xhr.ontimeout=ajaxto; return xhr;}\n"
        "function ajaxstr(){mtop(); btnsdis(true); setvis(loader,true); setinfo(''); setvis(popdiv,false);}\n"
        "function ajaxend(){mtop(); btnsdis(false); setvis(loader,false);}\n"
        "function ajaxres(txt){let res = jsonres(txt); if(!res) return; setinfo('Service#:&nbsp;' + res.svn); setpop(res.msg,res.clr);}\n"
        "function ajaxerr(err){setpop('Request failed! Status: ' + err + ' Try reloading the page','#AA0000');}\n"
        "function ajaxto(){setpop('Request timeout!','#AA0000'); ajaxend();}\n"
        "function ajaxrdy(){if(this.readyState != 4) return; if(this.status == 200) ajaxres(this.responseText); else ajaxerr(this.status); ajaxend();}\n"
        "function ajaxget(ref){ajaxstr(); var xhr = ajaxnew(); xhr.open('get',ref); xhr.send();}\n"
        "function ajaxsub(form){ajaxstr(); var xhr = ajaxnew(); xhr.open('post',form.action); xhr.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); xhr.send(formstr(form));}\n"
        "</script>\n\n"
        "</body>\n"
        "</html>\n";

    COMPILER_ASSERT(sizeof(footer_part_1) < MAX_CONTENT_SIZE, "footer_part_1 is bigger than max content size");
    _server.sendContent_P(footer_part_1);
    COMPILER_ASSERT(sizeof(footer_part_2) < MAX_CONTENT_SIZE, "footer_part_2 is bigger than max content size");
    _server.sendContent_P(footer_part_2);

    _server.sendContent(""); // end chunked page
}

void PageManager::send_root_page()
{
    static const char root_content[] PROGMEM = "<p>\n"
                                               "Welcome to ESP8266 SSTC interrupter server<br>\n"
                                               "Choose your action from the menu above\n"
                                               "</p>\n\n";

    _start_chunked_page("Welcome");

    COMPILER_ASSERT(sizeof(root_content) < MAX_CONTENT_SIZE, "root page content is bigger than max content size");
    _server.sendContent_P(root_content);

    _end_chunked_page();
}

void PageManager::send_config_page()
{
    String content;

    _start_chunked_page("Configuration");

    content = ("<h3>Network:</h3>\n"
               "<hr>\n"
               "<form method=\"post\" action=\"" HREF_SET_CONFIG "\">\n" +
               FORM_INPUT_TEXT("Network Name", String(SavedConfig::FORM_KEY_NET_SSID), _config.net_ssid()) +
               FORM_INPUT_TEXT("Network Password", String(SavedConfig::FORM_KEY_NET_PASS), _config.net_pass()) +
               FORM_INPUT_TEXT("Access Point Name", String(SavedConfig::FORM_KEY_AP_SSID), _config.ap_ssid()) +
               FORM_INPUT_TEXT("Access Point Password", String(SavedConfig::FORM_KEY_AP_PASS), _config.ap_pass()) +
               FORM_INPUT_TEXT("Authentication User", String(SavedConfig::FORM_KEY_AUTH_USER), _config.auth_user()) +
               FORM_INPUT_TEXT("Authentication Password", String(SavedConfig::FORM_KEY_AUTH_PASS), _config.auth_pass()) +
               FORM_INPUT_TEXT("mDNS Name", String(SavedConfig::FORM_KEY_MDNS_NAME), _config.mdns_name()));

    BUG(content.length() > MAX_CONTENT_SIZE);
    _server.sendContent(content);

    content = (FORM_INPUT_TEXT_IP("Static IP", String(SavedConfig::FORM_KEY_STATIC_IP), _config.static_ip()) +
               FORM_INPUT_TEXT_IP("Subnet Mask", String(SavedConfig::FORM_KEY_SUBNET), _config.subnet()) +
               FORM_INPUT_TEXT_IP("Default Gateway IP", String(SavedConfig::FORM_KEY_GATEWAY), _config.gateway()) +
               FORM_INPUT_TEXT_IP("DNS IP", String(SavedConfig::FORM_KEY_DNS), _config.dns()) +
               FORM_INPUT_TEXT_DISABLED("Access Point Server IP", String(SavedConfig::FORM_KEY_DNS), String("192.168.4.1")) +
               "<hr>\n"
               "<h3>Interrupter:</h3>\n"
               "<hr>\n" +
               FORM_INPUT_NUMBER_INT("Max PWM frequency", String(SavedConfig::FORM_KEY_MAX_FREQ), _config.max_freq(), PWM_MIN_FREQ, PWM_MAX_FREQ, "Hz") +
               FORM_INPUT_NUMBER_INT("Max PWM width", String(SavedConfig::FORM_KEY_MAX_WIDTH), _config.max_width(), PWM_MIN_WIDTH, PWM_MAX_WIDTH, "us") +
               FORM_INPUT_NUMBER("Max PWM duty cycle", String(SavedConfig::FORM_KEY_MAX_DUTY), _config.max_duty(), 1, 100, "%") +
               FORM_INPUT_NUMBER_INT("Max PWM duration", String(SavedConfig::FORM_KEY_MAX_DURATION), (_config.max_duration()), 1000, 3600000, "ms") +
               "<hr>\n"
               "</form>\n"
               "<br>\n"
               "<div class=\"submenu\">\n"
               "<button class=\"gbtn\" onclick=\"ajaxsub(document.forms[0])\">Save Configuration</button>\n"
               "</div>\n"
               "<br><br><br><br>\n\n");

    BUG(content.length() > MAX_CONTENT_SIZE);
    _server.sendContent(content);

    _end_chunked_page();
}

void PageManager::send_control_page()
{
    static const char control_script_1[] PROGMEM =
        "<script>\n"
        "var ifreq=document.getElementById(\"ifreq\");\n"
        "var ofreq=document.getElementById(\"ofreq\");\n"
        "var iwidth=document.getElementById(\"iwidth\");\n"
        "var owidth=document.getElementById(\"owidth\");\n"
        "var iduty=document.getElementById(\"iduty\");\n"
        "var oduty=document.getElementById(\"oduty\");\n"
        "var idur=document.getElementById(\"idur\");\n"
        "var odur=document.getElementById(\"odur\");\n"
        "var istrt=document.getElementById(\"istrt\");\n"
        "function updt() {\n"
        "\tofreq.innerHTML=ifreq.value;\n"
        "\towidth.innerHTML=iwidth.value;\n"
        "\toduty.innerHTML=iduty.value;\n"
        "\todur.innerHTML=idur.value;\n\n"
        "\tif(ifreq.value != 0 || iwidth.value != iwidth.max) {\n"
        "\t\tistrt.innerHTML=\"Start PWM\"\n"
        "\t\tistrt.classList.remove(\"rbtn\"); istrt.classList.add(\"gbtn\");\n"
        "\t} else {\n"
        "\t\tistrt.innerHTML=\"Start CW\"\n"
        "\t\tistrt.classList.remove(\"gbtn\"); istrt.classList.add(\"rbtn\");\n"
        "\t}\n"
        "}\n\n"
        "\tfunction cpd() {\n"
        "\tlet cp = 1000000 / ifreq.value;\n"
        "\tlet cd = 100 * iwidth.value / cp;\n"
        "\treturn {p:cp,d:cd};\n"
        "}\n\n";

    static const char control_script_2[] PROGMEM =
        "function sduty() {\n"
        "\tif(ifreq.value == 0) {\n"
        "\t\tiduty.value = 0;\n"
        "\t\treturn;\n"
        "\t}\n"
        "\tlet pd = cpd();\n"
        "\tif (pd.d <= iduty.max) {\n"
        "\t\tiduty.value = pd.d;\n"
        "\t\treturn;\n"
        "\t}\n"
        "\tlet fw = iduty.max * pd.p / 100;\n"
        "\tif(fw <= iwidth.max) {\n"
        "\t\tiduty.value = iduty.max;\n"
        "\t\tiwidth.value = fw;\n"
        "\t} else {\n"
        "\t\tiwidth.value = iwidth.max;\n"
        "\t\tiduty.value = 100 * iwidth.max / pd.p;\n"
        "\t}\n"
        "}\n\n"
        "ifreq.oninput=function() {\n"
        "\tsduty();\n"
        "\tif(ifreq.value == 0)\n"
        "\t\tiwidth.value = 0;\n"
        "\tupdt();\n"
        "}\n\n"
        "iwidth.oninput=function() {\n"
        "\tsduty();\n"
        "\tupdt();\n"
        "}\n\n"
        "iduty.oninput=function() {\n"
        "\tif(ifreq.value == 0) {\n"
        "\t\tiduty.value = 0;\n"
        "\t} else {\n"
        "\t\tlet pd = cpd();\n"
        "\t\tfw = iduty.value * pd.p / 100;\n"
        "\t\tif(fw <= iwidth.max) {\n"
        "\t\t\tiwidth.value = fw;\n"
        "\t\t} else {\n"
        "\t\t\tiwidth.value = iwidth.max;\n"
        "\t\t\tiduty.value = 100 * iwidth.max / pd.p;\n"
        "\t\t}\n"
        "\t}\n"
        "\tupdt();\n"
        "}\n\n"
        "idur.oninput=function() {\n"
        "\tupdt();\n"
        "}\n\n"
        "sduty();\n"
        "updt();\n\n"
        "</script>\n\n";

    String content;

    _start_chunked_page("Control");

    content = ("<form action=\"" HREF_PWM_START "\">\n" +
               FORM_INPUT_RANGE("PWM Frequency", "ifreq", "ofreq", String(PWMController::FORM_KEY_PWM_FREQ), String(_control.pwm_freq()), 0, String(_config.max_freq()), 1, "Hz") +
               FORM_INPUT_RANGE("PWM Width", "iwidth", "owidth", String(PWMController::FORM_KEY_PWM_WIDTH), String(_control.pwm_width()), 0, String(_config.max_width()), 1, "us") +
               FORM_INPUT_RANGE("PWM Duty Cycle", "iduty", "oduty", String(PWMController::FORM_KEY_PWM_DUTY), String(_control.pwm_duty()), 0, String(_config.max_duty()), 0.1, "%") +
               FORM_INPUT_RANGE("PWM Duration", "idur", "odur", String(PWMController::FORM_KEY_PWM_DURATION), String((float)_control.pwm_duration() / 1000), 0, String((float)_config.max_duration() / 1000), 0.1, "s") +
               "<hr>\n"
               "</form>\n"
               "<br>\n"
               "<div class=\"submenu\">\n"
               "<button class=\"gbtn\" id=\"istrt\" onclick=\"ajaxsub(document.forms[0])\">Start</button>\n"
               "</div>\n"
               "<br><br><br><br>\n\n");

    BUG(content.length() > MAX_CONTENT_SIZE);
    _server.sendContent(content);

    COMPILER_ASSERT(sizeof(control_script_1) < MAX_CONTENT_SIZE, "control_script_1 is bigger than max content size");
    _server.sendContent_P(control_script_1);

    COMPILER_ASSERT(sizeof(control_script_2) < MAX_CONTENT_SIZE, "control_script_2 is bigger than max content size");
    _server.sendContent_P(control_script_2);

    _end_chunked_page();
}

void PageManager::send_response(const PopMessage &msg)
{
    StaticJsonDocument<512> res;

    String label;
    String color;
    String text;
    String json_res;

    ++_service_count;

    text = msg.str.substring(0, 384); // truncate string to fit into allocated json memory

    switch (msg.type)
    {
    default:
    case PopMessage::MSG_INFO:
        color = "#009900";
        break;
    case PopMessage::MSG_WARNING:
        label = "WARNING: ";
        color = "#e6b800";
        break;
    case PopMessage::MSG_ERROR:
        label = "ERROR: ";
        color = "#AA0000";
        break;
    }

    res["svn"] = _service_count;
    res["clr"] = color;
    res["msg"] = label + text;

    if (serializeJson(res, json_res) != 0)
    {
        _server.send(HTTP_OK, CONTENT_TYPE_JSON, json_res);
    }
    else
    {
        _server.send(HTTP_OK, CONTENT_TYPE_HTML, "Failed to write json response!");
    }
}
