#ifndef Engine_h__
#define Engine_h__

#include "registry.h"

#include <vector>
#include <map>
#include <typeinfo>
#include <functional>

class System;

class Engine
{
private:
    static Engine* _instance;
public:
    std::vector<System*> systems;
    Registry registry;

    static Engine* instance();

    void addSystem(System* sys);
    void removeSystem(System *sys);

    void start();
    void loop();
    Engine(/* args */);
    ~Engine();
    
void init();
};
 
#endif // Engine_h__
