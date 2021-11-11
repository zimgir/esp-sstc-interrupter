#include <Arduino.h>

#include "config.h"
#include "utils.h"

#include "PWMController.h"

PWMController::PWMController(const SavedConfig &config) : _config(config),
                                                          _is_active(false),
                                                          _t0(0),
                                                          _pwm_freq(100),
                                                          _pwm_width(200),
                                                          _pwm_duration(1000)
{
}

void PWMController::init()
{
    analogWriteRange(PWM_RANGE);
    stop();
}

String PWMController::limits_str()
{
    return ("Frequency: [" STR(PWM_MIN_FREQ) "," STR(PWM_MAX_FREQ) "] Hz | "
                                                                   "Width: [" STR(PWM_MIN_WIDTH) "," STR(PWM_MAX_WIDTH) "] us | "
                                                                                                                        "Duty Cycle: [" STR(0) "," +
            String(_config.max_duty()) + "] % | "
                                         "Duration: [" STR(0) "," +
            String(_config.max_duration()) + "] ms");
}

PWMController::StartResult PWMController::start()
{
    uint32_t period_us;
    uint32_t pwm_val;
    uint32_t duty;

    bool clipped = false;
    // limit power according to config
    if (_pwm_freq > PWM_MAX_FREQ)
    {
        _pwm_freq = PWM_MAX_FREQ;
        clipped = true;
    }

    if (_pwm_freq < PWM_MIN_FREQ)
    {
        // no clamp because we need to check for 0 frequency for CW triggering
        // frequency will be clamped in analogWriteFreq anyway
        //_pwm_freq = PWM_MIN_FREQ; 
        clipped = true;
    }

    if (_pwm_width > PWM_MAX_WIDTH)
    {
        _pwm_width = PWM_MAX_WIDTH;
        clipped = true;
    }

    if (_pwm_width < PWM_MIN_WIDTH)
    {
        _pwm_width = PWM_MIN_WIDTH;
        clipped = true;
    }

    if (_pwm_duration > _config.max_duration())
    {
        _pwm_duration = _config.max_duration();
        clipped = true;
    }

    // energy zeros
    if (_pwm_width <= 0 || _pwm_duration <= 0)
    {
        stop();
        return START_OFF;
    }

    if (_pwm_freq != 0)
    {
        // general PWM setup
        period_us = 1000000 / _pwm_freq;
        duty = 100 * _pwm_width / period_us;

        // consider max duty cycle restriction
        if (duty > _config.max_duty())
        {
            _pwm_width = _config.max_duty() * period_us / 100;
            clipped = true;
        }

        pwm_val = PWM_RANGE * _pwm_width / period_us;
    }
    else
    {
        // enable triggering CW mode with 0 frequency and max width
        if (_pwm_width >= _config.max_width())
        {
            pwm_val = PWM_RANGE;
        }
        else
        {
            pwm_val = 0;
        }
    }

    if (pwm_val <= 0)
    {
        stop();
        return START_OFF;
    }

    _is_active = true;
    _t0 = millis();

    if (pwm_val < PWM_RANGE)
    {
        // set width to 0 before changing frequency - jittery but safe
        analogWrite(PIN_OUTPUT, 0);
        analogWriteFreq(_pwm_freq);
        analogWrite(PIN_OUTPUT, pwm_val);
        return clipped ? START_PWM_CLIPPED : START_PWM;
    }

    digitalWrite(PIN_OUTPUT, HIGH);
    return START_CW;
}

void PWMController::stop()
{
    digitalWrite(PIN_OUTPUT, LOW);
    _is_active = false;
}

void PWMController::loop()
{
    if (!_is_active)
        return;

    if ((millis() - _t0) >= _pwm_duration)
        stop();
}

enum FormInterface::SetResult PWMController::set(const String &key, const String &val, String &msg)
{
    uint32_t parsed_int;
    float parsed_float;
    char msgbuf[128];

    parsed_int = (uint32_t)val.toInt();

    switch (key.toInt())
    {
    case FORM_KEY_PWM_FREQ:
    {
        if (parsed_int > _config.max_freq())
        {
            snprintf(msgbuf, sizeof(msgbuf), "PWM frequency: %u is invalid! Max: %u [Hz]",
                     parsed_int, _config.max_freq());
            msg = msgbuf;
            return SET_INVALID_VALUE;
        }

        _pwm_freq = parsed_int;

        break;
    }
    case FORM_KEY_PWM_WIDTH:
    {
        if (parsed_int > _config.max_width())
        {
            snprintf(msgbuf, sizeof(msgbuf), "PWM width: %u is invalid! Max: %u [us]",
                     parsed_int, _config.max_width());
            msg = msgbuf;
            return SET_INVALID_VALUE;
        }

        _pwm_width = parsed_int;

        break;
    }
    case FORM_KEY_PWM_DUTY:
    {
        parsed_float = val.toFloat();
        if (parsed_float > _config.max_duty())
        {
            snprintf(msgbuf, sizeof(msgbuf), "PWM duty cycle: %.1f is invalid! Max: %.1f [%%]",
                     parsed_float, _config.max_duty());
            msg = msgbuf;
            return SET_INVALID_VALUE;
        }

        _pwm_duty = parsed_float;

        break;
    }
    case FORM_KEY_PWM_DURATION:
    {
        parsed_float = val.toFloat();

        // convert float seonds to ms
        parsed_int = 1000 * parsed_float;

        if (parsed_int > _config.max_duration())
        {
            snprintf(msgbuf, sizeof(msgbuf), "PWM duration: %u is invalid! Max: %u [ms]",
                     parsed_int, _config.max_duration());
            msg = msgbuf;
            return SET_INVALID_VALUE;
        }

        _pwm_duration = parsed_int;

        break;
    }
    default:
        return SET_INVALID_KEY;
    }

    return SET_OK;
}
