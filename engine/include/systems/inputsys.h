#ifndef inputsys_h__
#define inputsys_h__
#include "System.h"

class InputSys : public System
{
  public:
    InputSys();
    ~InputSys();

    void start() override;

    void process() override;

  private:
};

#endif // inputsys_h__
