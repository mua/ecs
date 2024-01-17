#ifndef core_h__
#define core_h__

#include "types.h"
#include <array>
#include <string>

#include "storage.h"
#include "serialize.h"

struct Info
{
    std::string name;
    int scope=0;
    bool active = true;

    static Encoder serializer()
    {
        return serialize(
            "name", &Info::name, 
            "scope", &Info::scope, 
            "active", &Info::active);
    }
};
REGISTER_COMPONENT(Info)

struct Ref
{
    std::string path;

    static Encoder serializer()
    {
        return serialize("ref", &Ref::path);
    }
};
REGISTER_COMPONENT(Ref)

struct Proto
{
    std::string path;
    static Encoder serializer()
    {
        return serialize("proto", &Proto::path);
    }
};
REGISTER_COMPONENT(Proto)

struct Instance
{
};
REGISTER_COMPONENT(Instance)

struct Relation
{
    Handle parent;
    std::vector<Handle> children;

    void add(Handle handle)
    {      
        children.push_back(handle);
    }
    static Encoder serializer()
    {
        return serialize(
            "parent", &Relation::parent, 
            "children", &Relation::children);
    }
};
REGISTER_COMPONENT(Relation)

#endif // core_h__