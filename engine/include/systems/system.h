#ifndef System_h__
#define System_h__

#include <functional>
#include "Types.h"

#include "registry.h"
#include "engine.h"

class System
{
  private:
    /* data */    
  public:
    Registry *r;

    virtual void start() = 0;
    virtual void process() = 0;


    template <class C, class T>
    void syncResource(std::function<T(Handle entity, C &)> create, std::function<bool(C &)> filter = nullptr,
                      std::function<void(C&, T&)> update = nullptr)
    {
        r->each<C>([&](int entity, C &target) {
            if (!filter || filter(target))
            {
                if (!r->has<T>(entity))
                {
                    r->addComponent(entity, create(entity, target));
                }
                else
                {
                    if (update)
                    {
                        update(target, r->get<T>(entity));
                    }
                }
            }
        });
    }
};
#endif // System_h__
