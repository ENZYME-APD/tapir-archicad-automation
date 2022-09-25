#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['Link', 'Parameter', 'Command', 'CommandResult']

# - - - - - - - - BUILT-IN IMPORTS
import json
import time
import sys
if sys.version_info.major == 3:
    from urllib.request import urlopen, Request
else:
    from urllib2 import urlopen, Request

# - - - - - - - - LOCAL IMPORTS
from tapir_py.utility import dotNETBase, JsonExtensions
from tapir_py.parts import Element, ClassificationSystem, BoundingBox

# - - - - - - - - CLASS LIBRARY
class Link(dotNETBase):

    _PORT = range(19723, 19743)
    _HOST = 'http://127.0.0.1'
    
    @property
    def port(self):
        return self._port

    @port.setter
    def port(self, port):
        if Link.is_valid(port):
            self._port = port
        else:
            raise Exception('Invalid port')
    
    @property
    def host(self):
        return self._HOST

    @property
    def address(self):
        return '{}:{}'.format(self.host, self.port)
    
    def __init__(self, port=19723):
        self.port = port

    @staticmethod
    def is_valid(port):
        return port in Link._PORT

    def is_alive(self):
        return Command(self).IsAlive()

    def post(self, data):
        connection_object = Request(self.address)
        connection_object.add_header('Content-Type', 'application/json')
        
        start_time = time.time()
        response = urlopen(connection_object, json.dumps(data).encode('utf8'))
        print('Completed in {:.3f} seconds'.format(time.time() - start_time))
        return CommandResult(response.read())

    def __str__(self):
        return 'TapirLink : {}'.format(self.address)

class CommandResult(dotNETBase):
    
    def __init__(self, response):
        #TODO: Check if deepcopy is necessary here
        self._data = json.loads(response, object_hook=JsonExtensions.strip_unicode)
        self.success = self._data.get('succeeded')
        self.result = self._data.get('result', {})
        self.error = self._data.get('error', {})

    def bounding_box(self):
        return BoundingBox.from_command_result(self.result)
    
    def classification_systems(self):
        return ClassificationSystem.from_command_result(self.result)
    
    def exception(self):
        if self.error is not None:
            return Exception('{}:\n{}'.format(self.error['code'], self.error['message']))

    def elements(self):
        return Element.from_command_result(self.result)
    
    def __str__(self):
        return '{} CommandResult'.format('Success' if self.success else 'Failed')

class Parameter(dotNETBase):

    @staticmethod
    def pack(self, parameters):
        if all([isinstance(param, Parameter) for param in parameters]):
            return {'parameters': parameters}
    
    def __init__(self, key, value):
        self.key = key
        self.value = value
    
    def ToDictionary(self):
        return {'parameters' : {self.key: self.value}}
    
    def __str__(self):
        return '<{} : {}>'.format(self.key, self.value)

class Command(dotNETBase):
    
    @classmethod
    def create(cls, port=19723):
        return cls(Link(port))
    
    def __init__(self, link):
        self.link = link

    def __str__(self):
        return 'ArchiCAD Command Object'

    #region Basic Commands
    def IsAlive(self):
        cmd = {'command' : 'API.IsAlive'}
        try:
            response = self.link.post(cmd)
            return response.success
        except Exception as ex:
            if type(ex).__name__ == 'URLError':
                return False
            else:
                raise ex

    def GetProductInfo(self):
        cmd = {'command' : 'API.GetProductInfo'}
        response = self.link.post(cmd)
        if response.success:
            return response.result["version"], response.result["buildNumber"], response.result["languageCode"]
        else:
            raise response.exception()
    #endregion Basic Commands

    #region Element Listing Commands
    def GetAllElements(self):
        cmd = {'command' : 'API.GetAllElements'}
        response = self.link.post(cmd)
        if response.success:
            return response.elements()
        else:
            raise response.exception()

    def GetElementsByType(self, element_type):
        
        for item in Element._TYPES:
            if item.lower() == element_type.lower():
                element_type = item
                break
        else:
            raise TypeError("Couldn't find element type: {}".format(element_type))
        
        cmd = { 'command' : 'API.GetElementsByType',
                'parameters' : {'elementType': element_type}
                }
        response = self.link.post(cmd)
        if response.success:
            return response.elements()
        else:
            raise response.exception()

    def GetElementsByClassification(self, classification_system_id):
        cmd = { 'command' : 'API.GetElementsByClassification',
                'parameters' : { 'classificationSystemId' : {'guid' : classification_system_id}}
                }
        response = self.link.post(cmd)
        if response.success:
            return response.elements()
        else:
            raise response.exception()
    #endregion Element Listing Commands

    #region Classification Commands
    def GetAllClassificationSystems(self):
        cmd = {'command' : 'API.GetAllClassificationSystems'}
        response = self.link.post(cmd)
        if response.success:
            return response.classification_systems()
        else:
            raise response.exception()
    #endregion Classification Commands

    #region Element Geometry Commands
    def Get2DBoundingBoxes(self, elements):
        element_list = []
        for element in elements:
            element_list.append(element.ToDictionary())
        
        cmd = {'command' : 'API.Get2DBoundingBoxes'}
        cmd.update(Parameter('elements', element_list).ToDictionary())
        response = self.link.post(cmd)
        if response.success:
            return response.bounding_box()
        else:
            raise response.exception()
    
    def Get3DBoundingBoxes(self, elements):
        element_list = []
        for element in elements:
            element_list.append(element.ToDictionary())
        
        cmd = {'command' : 'API.Get3DBoundingBoxes'}
        cmd.update(Parameter('elements', element_list).ToDictionary())
        response = self.link.post(cmd)
        if response.success:
            return response.bounding_box()
        else:
            raise response.exception()
    #endregion Element Geometry Commands
