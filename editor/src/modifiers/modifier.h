#ifndef modifier_h__
#define modifier_h__
#include "types.h"


struct Modifier
{
    bool enabled = true;
    virtual bool process(Handle viewport) = 0;
    virtual bool isAvailable()=0;
};


#endif // modifier_h__
