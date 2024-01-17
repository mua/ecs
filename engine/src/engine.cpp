#include "Engine.h"

#include "Systems/System.h"
#include "systems/ResourceSys.h"
#include <algorithm>


Engine* Engine::_instance = nullptr;

Engine* Engine::instance()
{
	if (!Engine::_instance)
	{
		Engine::_instance = new Engine();
        Engine::_instance->init();
	}
	return Engine::_instance;
}

void Engine::addSystem(System* sys)
{
	systems.push_back(sys);
}

void Engine::start()
{
	for (auto sys: systems)
	{
        sys->r = &registry;
		sys->start();
	}
}

void Engine::loop()
{
	for (int i=0; i<systems.size(); i++)
	{
        auto sys = systems[i];
        if (sys)
        {
            sys->process();
            registry.cleanUp();
        }
	}
}

Engine::Engine(/* args */)
{
}

Engine::~Engine()
{
}

void Engine::init()
{
    addSystem(ResourceSys::instance());
}

void Engine::removeSystem(System* sys)
{
    std::replace(systems.begin(), systems.end(), sys, (System*)nullptr);
}
