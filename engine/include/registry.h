#ifndef Registry_h__
#define Registry_h__

#include "Storage.h"

#include <functional>
#include <map>

struct EntityInfo
{
    Handle handle;
    std::vector<std::string> components;
};

class Registry
{
  private:
    std::map<std::string, StorageBase *> storages;
    int entityCounter = 1;
    std::vector<Handle> released;

  public:
    Registry(/* args */);
    ~Registry();

    void cleanUp();
	
    StorageBase *storage(const std::string& typeName)
    {
        std::string id = typeName;
        if (!storages.count(id))
        {
            storages[id] = StorageBase::create(typeName);
        }
        return storages[id];
    }

    template <class T> Storage<T> *storage()
    {
        std::string id = typeid(T).name();
        return static_cast<Storage<T> *>(storage(id));
    }

    Handle nextEntityId()
    {
        return entityCounter++;
    }

    Handle copy(Handle source, Handle target)
    {
        for (auto storage : storages)
        {
            storage.second->copy(source, target);
        }
        return target;
    }

    void copyFrom(Registry& reg)
    {
        for (auto st : reg.storages)
        {
            storages[st.first] = st.second->clone();
        }
        entityCounter = reg.entityCounter;
    }

    template <class T> void addComponent(int entityId, T v)
    {
        storage<T>()->add(entityId, v);
    };

    template <class T> void removeComponent(int entityId)
    {
        storage<T>()->remove(entityId);
    };

    template <typename T, typename... Args> void addComponent(int entityId, T v, Args... args)
    {
        storage<T>()->add(entityId, v);
        addComponent(entityId, args...);
    };

    template <typename T, typename... Args> Handle createEntity(T v, Args... args)
    {
        Handle entity = nextEntityId();
        addComponent(entity, v, args...);
        return entity;
    };

    void removeEntity(Handle handle)
    {
        for (auto storage : storages)
        {
            storage.second->remove(handle);
        }
    }

    template <typename First, typename F> void entity_impl(int id, F &&callback)
    {
        auto comp = storage<First>()->get(id);
        if (comp)            
            callback(*comp);
    }

    template <typename First, typename Second, typename... Rest, typename F> void entity_impl(int id, F &&callback)
    {
        auto comp = storage<First>()->get(id);
        if (!comp)
            return;
        entity_impl<Second, Rest...>(id, [&](Second &sec, Rest &... args) { callback(*comp, sec, args...); });
    }

    template <typename... Rest, typename F> void entity(int id, F &&callback)
    {
        entity_impl<Rest...>(id, std::function<void(Rest & ...)>(callback));
    }

    template <typename First> const std::tuple<First &> getEntity(int id)
    {
        First &comp = *storage<First>()->get(id);
        return std::forward_as_tuple<First &>(comp);
    }

    template <typename First, typename Second, typename... Args>
    const std::tuple<First &, Second &, Args &...> getEntity(int id)
    {
        First &comp = *storage<First>()->get(id);
        return std::tuple_cat(std::forward_as_tuple<First &>(comp), getEntity<Second, Args...>(id));
    }

    template <typename T> bool has(int id)
    {
        return storage<T>()->get(id) != nullptr;
    }

    template <typename T> bool get(int id, T &val)
    {
        if (auto comp = storage<T>()->get(id))
        {
            val = *comp;
            return true;
        }
        return false;
    }

    template <typename T> std::vector<Handle> findAll()
    {
        return storage<T>()->entities();
    }

    template <typename T> std::map<Handle, T*> get(std::vector<Handle> entities)
    {
        auto ret = std::map<Handle, T *>();
        for (auto handle: entities)
        {
            if (auto c = storage<T>()->get(handle))
                ret[handle] = c;
        }
        return ret;
    }

    template <typename T> T &get(int id)
    {
        auto ptr = storage<T>()->get(id);
        if (!ptr)
        {
            auto v = T();
            storage<T>()->add(id, v);
            ptr = storage<T>()->get(id);
        }
        return *ptr;
    }

    template <typename T> T* getPtr(int id)
    {
        return storage<T>()->get(id);
    }

    template <typename T, typename... Rest> bool get(int id, T &val, Rest &... args)
    {
        if (auto comp = storage<T>()->get(id))
        {
            val = *comp;
            return get<Rest...>(id, args...);
        }
        return false;
    }

    template <typename T, typename S, typename... Rest, typename F> void each(F &&callback)
    {
        storage<T>()->forEach(
            [&](int id, T &value) { 
                entity<S, Rest...>(id, [&](S& value2, Rest &... args) { 
                    callback(id, value, value2, args...); 
                }); 
            });
    }

    template <typename T, typename F> void each(F&& callback)
    {
        storage<T>()->forEach([&](int id, T &value) { callback(id, value); });
    }
    std::map<Handle, EntityInfo> entities();
    void release(Handle entity);
    void toJson(std::vector<int> &entities, json &out);
    void fromJson(json &j);
};
#endif // Registry_h__
