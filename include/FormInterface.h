#ifndef __FORM_INTERFACE_H__
#define __FORM_INTERFACE_H__

#include <Arduino.h>


class FormInterface
{

public:

    FormInterface() {}
    ~FormInterface() {}

    enum SetResult
    {
        SET_OK,
        SET_INVALID_KEY,
        SET_VAL_TOO_LONG,
        SET_INVALID_VALUE,
    };


    virtual enum SetResult set(const String& key, const String &val, String& msg) = 0;

    


};

#endif