#ifndef transformsys_h__
#define transformsys_h__

#include "system.h"

class TransformSys : public System
{
  public:
    void start() override;
    void process() override;
};

#endif // transformsys_h__
