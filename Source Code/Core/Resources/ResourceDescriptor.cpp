#include "Headers/ResourceDescriptor.h"

ResourceDescriptor::ResourceDescriptor(const std::string& name,
                                       const std::string& resourceLocation,
                                       bool flag, U32 id, U8 enumValue) : _propertyDescriptor(NULL),
                                                                          _name(name),
                                                                          _resourceLocation(resourceLocation),
                                                                          _flag(flag),
                                                                          _id(id),
                                                                          _enumValue(enumValue)
{
    _mask.i = 0;
    _threaded = true;
}

ResourceDescriptor::~ResourceDescriptor()
{
    SAFE_DELETE(_propertyDescriptor);
}

ResourceDescriptor::ResourceDescriptor(const ResourceDescriptor& old) : _propertyDescriptor(NULL)
{
    _name = old._name;
    _resourceLocation = old._resourceLocation;
    _properties = old._properties;
    _flag = old._flag;
    _threaded = old._threaded;
    _id = old._id;
    _mask = old._mask;
    _enumValue = old._enumValue;
    if(old._propertyDescriptor != NULL)	_propertyDescriptor = old._propertyDescriptor->clone();
}

 ResourceDescriptor& ResourceDescriptor::operator= (ResourceDescriptor const& old) {
    if (this != &old) {
        _name = old._name;
        _resourceLocation = old._resourceLocation;
        _properties = old._properties;
        _flag = old._flag;
        _threaded = old._threaded;
        _id = old._id;
        _mask = old._mask;
        _enumValue = old._enumValue;
        if(old._propertyDescriptor != NULL)	 SAFE_UPDATE(_propertyDescriptor, old._propertyDescriptor->clone());
      }

      return *this;
}