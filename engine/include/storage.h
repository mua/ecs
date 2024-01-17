#ifndef Storage_h__
#define Storage_h__

#include "types.h"

#include <map>
#include <functional>

#include "serialize.h"

class StorageBase
{
  private:
    static std::map<std::string, std::function<StorageBase *()>> *_constructors;
  public:
    virtual void copy(Handle source, Handle target) = 0;
    virtual std::vector<Handle> entities() = 0;
    virtual std::string description() = 0;
    virtual void remove(int entityId) = 0;
    virtual StorageBase *clone()=0;

    
    static std::map<std::string, std::function<StorageBase*()>> *constructors();
    static StorageBase *create(std::string typeName);
    template <class T> 
    static bool registerComponent(); 

    virtual bool isSerializable() = 0;
    virtual bool has(Handle entity)=0;
    virtual void serialize(Handle entity, json& j) = 0;
    virtual void unserialize(Handle entity, json &j) = 0;
    virtual std::string componentTypeName()=0;
    virtual void add(int entityId, void *v) = 0;    
};


template<class C>
class Storage :public StorageBase
{
private:
    std::map<int, C> componentMap;
public:
    void add(int entityId, C& v)
    {
        componentMap[entityId] = v;
    };

    virtual void add(int entityId, void *v)
    {
        componentMap[entityId] = *((C*)(v));
    }

    virtual bool isSerializable() override
    {
        if constexpr (::isSerializable<C>())
        {
            return true;
        }
        return false;
    }

    virtual void serialize(Handle entity, json &j)
    {
        if constexpr (::isSerializable<C>())
        {
            C::serializer().serialize(&componentMap[entity], j);
        }
    }

    virtual void unserialize(Handle entity, json &j)
    {
        if constexpr (::isSerializable<C>())
        {
            C::serializer().unserialize(&componentMap[entity], j);
        }
    }

    virtual std::string componentTypeName()
    {
        return typeid(C).name();
    }

    StorageBase* clone()
    {
        auto cpy = new Storage<C>();
        cpy->componentMap = componentMap;
        return cpy;
    }

    virtual void copy(Handle source, Handle target) override
    {
        if (componentMap.count(source))
            componentMap[target] = componentMap[source];
    }

    virtual void remove(int entityId) override
    {
        if (componentMap.count(entityId))
            componentMap.erase(entityId);
    }

    C* get(int entityId)
    {
        return componentMap.count(entityId) > 0 ? &componentMap[entityId] : nullptr;
    };

    virtual bool has(Handle entity)
    {
        return componentMap.count(entity);
    }

    void forEach(std::function<void(int entityId, C& value)> fn)
    {
        for (auto& v : componentMap)
        {
            fn(v.first, v.second);
        }
    };

    virtual std::vector<Handle> entities() override
    {
        auto ret = std::vector<Handle>();
        for (auto v : componentMap)
        {
            ret.push_back(v.first);
        }
        return ret;
    }
     
    virtual std::string description()  override
    {
        return std::string(typeid(C).name()).replace(0, 6, "");
    }
};

template <class T> bool StorageBase::registerComponent()
{
    (*constructors())[typeid(T).name()] = []() { return new Storage<T>(); };
    return true;
}

#define REGISTER_COMPONENT(COMP) static bool result_##COMP = StorageBase::registerComponent<COMP>();
#endif // Storage_h__