#ifndef serialize_h__
#define serialize_h__

#include <utility>
#include <vector>

#include <json.hpp>

using json = nlohmann::json;

namespace glm
{
inline void to_json(json &j, const glm::vec2 &p)
{
    j = {p.x, p.y};
};

inline void from_json(const json &j, glm::vec2 &p)
{
    p.x = j.at(0).get<float>();
    p.y = j.at(1).get<float>();
}
inline void to_json(json &j, const glm::vec3 &p)
{
    j = {p.x, p.y, p.z};
};

inline void from_json(const json &j, glm::vec3 &p)
{
    p.x = j.at(0).get<float>();
    p.y = j.at(1).get<float>();
    p.z = j.at(2).get<float>();
}
inline void to_json(json &j, const glm::vec4 &p)
{
    j = {p.x, p.y, p.z, p.w};
};

inline void from_json(const json &j, glm::vec4 &p)
{
    p.x = j.at(0).get<float>();
    p.y = j.at(1).get<float>();
    p.z = j.at(2).get<float>();
    p.w = j.at(3).get<float>();
}
} // namespace glm

struct SerializerBase
{
    std::string name;

    SerializerBase(std::string name) : name(name)
    {
    }

    virtual void serialize(void *obj, json &j)
    {
    }
    virtual void unserialize(void *obj, const json &j)
    {
    }
};

struct Encoder
{
    std::vector<SerializerBase *> serializers;
    Encoder(std::vector<SerializerBase *> serializers) : serializers(serializers)
    {
    }

    void serialize(void *obj, json &j)
    {
        for (auto &s : serializers)
        {
            s->serialize(obj, j[s->name]);
        }
    }
    void unserialize(void *obj, const json &j)
    {
        for (auto &s : serializers)
        {
            s->unserialize(obj, j[s->name]);
        }
    }
};

template <typename T, typename Enable = void> struct SerializerFor
{
    void serializeValue(T &value, json &j)
    {
        j = json(value);
    }
    void unserializeValue(T &value, const json &j)
    {
        value = j.get<T>();
    }
};

template <typename T>
struct SerializerFor<T, std::enable_if_t<std::is_void<decltype(std::declval<T>().emplace_back(T::value_type))>::value>>
{
    void serializeValue(T &value, json &j)
    {
        auto sf = SerializerFor<typename T::value_type>();
        for (auto &v : value)
        {
            json jv;
            sf.serializeValue(v, jv);
            j.push_back(jv);
        }
    }
    void unserializeValue(T &value, const json &j)
    {
        auto sf = SerializerFor<typename T::value_type>();
        for (auto jv : j)
        {
            typename T::value_type v;
            sf.unserializeValue(v, jv);
            value.push_back(v);
        }
    }
};

template <typename T>
struct SerializerFor<T, std::enable_if_t<std::is_integral<decltype(std::declval<T>().count(T::key_type()))>::value>>
{
    void serializeValue(T &value, json &j)
    {
        for (auto kv : value)
        {
            SerializerFor<typename T::mapped_type>().serializeValue(kv.second, j[kv.first]);
        }
    }
    void unserializeValue(T &value, const json &j)
    {
        for (auto kv : json::iterator_wrapper(j))
        {
            SerializerFor<typename T::mapped_type>().unserializeValue(value[kv.key()], kv.value());
        }
    }
};

template <typename T>
struct SerializerFor<T, std::enable_if_t<std::is_same<decltype(std::declval<T>().serializer()), Encoder>::value>>
{
    void serializeValue(T &value, json &j)
    {
        T::serializer().serialize(&value, j);
    }
    void unserializeValue(T &value, const json &j)
    {
        T::serializer().unserialize(&value, j);
    }
};

template <typename C, typename T> struct Serializer : SerializerBase, SerializerFor<T>
{
    T C::*member;

    Serializer(const char *name, T C::*member) : SerializerBase(name), member(member)
    {
    }

    virtual void serialize(void *obj, json &j)
    {
        this->serializeValue((((C *)(obj))->*member), j);
    }
    virtual void unserialize(void *obj, const json &j)
    {
        this->unserializeValue((((C *)(obj))->*member), j);
    }
};

template <typename T, typename... Args> Encoder serialize(const char *name, T field, Args... args)
{
    Encoder encoder({new Serializer(name, field)});
    const auto &enc = serialize(args...);
    encoder.serializers.insert(encoder.serializers.end(), enc.serializers.begin(), enc.serializers.end());
    return encoder;
}

template <typename T> Encoder serialize(const char *name, T field)
{
    Encoder encoder({new Serializer(name, field)});
    return encoder;
}

template <typename T, typename = void> struct isSerializable : std::false_type
{
};
template <typename T>
struct isSerializable<T, std::enable_if_t<std::is_same<decltype(std::declval<T>().serializer()), Encoder>::value>>
    : std::true_type
{
};

#endif // serialize_h__