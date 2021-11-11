#ifndef __PWM_CONTROLLER_H__
#define __PWM_CONTROLLER_H__

#include <Arduino.h>

#include "SavedConfig.h"

class PWMController : public FormInterface
{
public:
    enum FormKey
    {
        FORM_KEY_PWM_FREQ = 200,
        FORM_KEY_PWM_WIDTH,
        FORM_KEY_PWM_DUTY,
        FORM_KEY_PWM_DURATION
    };

    enum StartResult
    {
        START_PWM,
        START_PWM_CLIPPED,
        START_OFF,
        START_CW
    };

    PWMController(const SavedConfig &config);
    ~PWMController() {}

    void init();
    void loop();

    StartResult start();
    void stop();

    String limits_str();

    enum FormInterface::SetResult set(const String &key, const String &val, String& msg) override;
 
    const uint32_t& pwm_freq() const { return _pwm_freq; }
    const uint32_t& pwm_width() const { return _pwm_width; }
    const uint32_t& pwm_duration() const { return _pwm_duration; }
    const float& pwm_duty() const {return _pwm_duty;}

    bool is_active() const {return _is_active; }

private:

    const SavedConfig& _config;

    bool _is_active;

    uint32_t _t0;
    uint32_t _pwm_freq;
    uint32_t _pwm_width;
    uint32_t _pwm_duration;
    float _pwm_duty;

};

#endif