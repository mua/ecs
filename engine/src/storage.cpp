#include "Storage.h"

std::map<std::string, std::function<StorageBase *()>> *StorageBase::_constructors = nullptr;

std::map<std::string, std::function<StorageBase *()>> *StorageBase::constructors()
{
    if (!StorageBase::_constructors)
    {
        StorageBase::_constructors = new std::map<std::string, std::function<StorageBase *()>>();
    }
    return _constructors;
}

StorageBase *StorageBase::create(std::string typeName)
{
    return (*StorageBase::constructors())[typeName]();
}
