#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['dotNETBase', 'RuntimeHelper', 'JsonExtensions']

# - - - - - - - - CLASS LIBRARY
class dotNETBase(object):
    """Base class for all classes that need to be compatible with .NET environment.

    """
    
    def __init__(self):
        raise NotImplementedError('Abstract base class cannot be instantiated.')
    
    def __str__(self):
        raise NotImplementedError('Abstract method cannot be called.')
    
    def __repr__(self):
        return self.__str__()
    
    def ToString(self):
        return self.__str__()
    
    def ToDictionary(self):
        raise NotImplementedError('Abstract method cannot be called.')

    @classmethod
    def FromDictionary(cls, json_data):
        raise NotImplementedError('Abstract method cannot be called.')

    def GetType(self):
        return self.__class__.__name__
    
    def GetArchicadType(self):
        name = self.__class__.__name__
        return name[0].lower() + name[1:]

class RuntimeHelper(dotNETBase):

    def __init__(self, data, pseudo_type):
        self.name = pseudo_type

        if isinstance(data, list):
            for item in data:
                self.__setattr__(str(item), item)

        if isinstance(data, dict):
            for key, value in data.items():
                self.__setattr__(key, value)
    
    def __str__(self):
        return self.name

class JsonExtensions:
    """A pseudo-static class that extends python's json module.

    """

    def __init__(self):
        raise NotImplementedError('Static class cannot be instantiated.')

    @staticmethod
    def strip_unicode(data):
        """Removes unicode string recursively.
        
        If input data contains unicode string, it is converted to UTF-8 encoding, making it compatible with Python 2.x and 3.x versions.

        Args:
            data (str | list[str] | dict): Data to be converted to UTF-8.

        Returns:
            str | list[str] | dict: Same as input.
        """

        if isinstance(data, str):
            return data

        if isinstance(data, list):
            return [JsonExtensions.strip_unicode(item) for item in data]

        if isinstance(data, dict):    
            return {JsonExtensions.strip_unicode(key): JsonExtensions.strip_unicode(value) for key, value in data.items()}

        if str(type(data).__name__) == 'unicode':
            return data.encode('utf-8')

        return data
