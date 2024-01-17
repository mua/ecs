#ifndef loadsys_h__
#define loadsys_h__

#include "systems/system.h"

class LoadSys : public System
{
  public:
    LoadSys();
    ~LoadSys();

    void start() override;

    void process() override;

  private:
    Handle loadGLTF(std::string path);
};
#endif // loadsys_h__