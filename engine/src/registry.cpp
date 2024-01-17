#include "Registry.h"
#include "Storage.h"

#include <unordered_set>

Registry::Registry(/* args */)
{
}

Registry::~Registry()
{
}

void Registry::cleanUp()
{
    for (auto entity : released)
    {
        for (auto storage : storages)
        {
            storage.second->remove(entity);
        }
    }
}

std::map<Handle, EntityInfo> Registry::entities()
{
    std::map<Handle, EntityInfo> set;

	for (auto kv: storages)
	{        
        for (auto handle : kv.second->entities())
        {
            set[handle].handle = handle;
            set[handle].components.push_back(kv.second->description());
        }
	}
    return set;
}

void Registry::release(Handle entity)
{
    released.push_back(entity);
}

void Registry::toJson(std::vector<int>& entities, json& out)
{
    out = json::object();
    for (auto entity : entities)
    {
        auto j = json::object();        
        for (auto stor : storages)
        {
            if (stor.second->isSerializable() && stor.second->has(entity))
            {                
                auto jc = json::object();
                stor.second->serialize(entity, jc);
                j[stor.second->componentTypeName()] = jc;
            }
        }
        out[std::to_string(entity)] = j;
    }
}

void Registry::fromJson(json &j)
{
    for (auto el: j.items())
    {        
        Handle entity = std::stoi(el.key());
        for (auto cel : el.value().items())
        {
            auto &val = cel.value();
            storage(cel.key())->unserialize(entity, val);
        }        
    }
}
