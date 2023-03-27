#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['Element', 'ClassificationSystem', 'BoundingBox']

# - - - - - - - - BUILT-IN IMPORTS
import sys

# - - - - - - - - LOCAL IMPORTS
if sys.version_info.major == 3:
    from .utility import dotNETBase, RuntimeObject
else:
    from utility import dotNETBase, RuntimeObject

# - - - - - - - - CLASS LIBRARY
class Element(dotNETBase):
    
    _TYPES = ['Wall', 'Column', 'Beam', 'Window', 'Door', 'Object', 'Lamp', 'Slab', 'Roof', 'Mesh', 'Zone', 'CurtainWall', 'Shell', 'Skylight', 'Morph', 'Stair', 'Railing', 'Opening']

    ELEMENT_TYPE = RuntimeObject(_TYPES, 'ElementTypeEnumerator')

    def __init__(self, guid):
        self.guid = guid
    
    def ToDictionary(self):
        return {'elementId' : {'guid' : self.guid} }

    @classmethod
    def FromDictionary(cls, json_data):
        id = json_data.get('elementId', {}).get('guid', '')
        if id: return cls(id)
    
    @staticmethod
    def list_from_command_result(result):
        return [Element.FromDictionary(data) for data in result.get('elements', [])]

    def __str__(self):
        return '<{} : {}>'.format(self.GetType(), self.guid)

class ClassificationSystem(dotNETBase):

    def __init__(self, guid, name, description, source, version, date):
        self.guid = guid
        self.name = name
        self.description = description
        self.source = source
        self.version = version
        self.date = date
    
    def ToDictionary(self):
        return {"classificationSystemId" : {"guid" : self.guid},
                "name" : self.name,
                "description" : self.description,
                "version" : self.version,
                "date" : self.date}

    @classmethod
    def FromDictionary(cls, json_data):
        if isinstance(json_data, dict):
            guid = json_data.get('classificationSystemId', {}).get('guid')
            name = json_data.get('name')
            description = json_data.get('description')
            source = json_data.get('source')
            version = json_data.get('version')
            date = json_data.get('date')
            return cls(guid, name, description, source, version, date)
        else:
            raise ValueError('json_data must be a dictionary')

    @staticmethod
    def list_from_command_result(result):
        return [ClassificationSystem.FromDictionary(data) for data in result.get('classificationSystems', [])]

    def __str__(self):
        return '<{} : {}>'.format(self.GetType(), self.name)

class ClassificationItem(dotNETBase):

    """ Represents a Classification Item
    Args:
        guid (str): A valid guid of classification item.
        id (str) : A short name used to identify the classification item.
        name (str) : A long name used to identify the classification item.
        description (str) : A description of classification item
        children (list) : A list of ClassificationItem
    """
    def __init__(self, guid, id, name, description, children=None):
        self.guid = guid
        self.id = id
        self.name = name
        self.description = description
        self.children = children
        
    def ToDictionary(self):
        return {'guid':self.guid,
                'id':self.id,
                'name':self.name,
                'description':self.description,
                'children':self.children}

    @classmethod
    def FromDictionary(cls, json_data):
        if isinstance(json_data, dict):
            itemData = json_data.get('classificationItem')
            guid = itemData.get('classificationItemId',{}).get('guid')
            id = itemData.get('id')
            name = itemData.get('name')
            description = itemData.get('description')
            children = []
            hasChildren = ('children' in itemData)
            if hasChildren:
                for data in itemData.get('children'):
                    childItem = ClassificationItem.FromDictionary(data)
                    children.append(childItem)
            return cls(guid,id,name,description,children)
        else:
            raise ValueError('json_data must be a dictionary')

    @staticmethod
    def list_from_command_result(result):
        return [ClassificationItem.FromDictionary(classItemData) for classItemData in result.get('classificationItems',[])]

    def __str__(self):
        return '<{} : {}>'.format(self.GetType(), self.name)

class BoundingBox(dotNETBase):

    @property
    def length(self):
        return self.x[1] - self.x[0]

    @property
    def breadth(self):
        return self.y[1] - self.y[0]

    @property
    def height(self):
        return self.z[1] - self.z[0]

    @property
    def is_2D(self):
        return self.height == 0

    @property
    def min_point(self):
        return (self.x[0], self.y[0], self.z[0])

    @property
    def max_point(self):
        return (self.x[1], self.y[1], self.z[1])

    def __init__(self, x_min, x_max, y_min, y_max, z_min, z_max):
        self.x = (float(x_min), float(x_max))
        self.y = (float(y_min), float(y_max))
        self.z = (float(z_min), float(z_max))

    def get_box_type(self):
        return '2D' if self.is_2D else '3D'

    def ToDictionary(self):
        
        if self.is_2D:
            return  {'boundingBox2D' : 
                        {
                        'xMin' : self.x[0],
                        'yMin' : self.y[0],
                        'xMax' : self.x[1],
                        'yMax' : self.y[1]}
                    }
        else:
            return  {'boundingBox3D' : 
                        {
                        'xMin' : self.x[0],
                        'yMin' : self.y[0],
                        'zMin' : self.z[0],
                        'xMax' : self.x[1],
                        'yMax' : self.y[1],
                        'zMax' : self.z[1]}
                    }

    @classmethod
    def FromDictionary(cls, json_data):
        if isinstance(json_data, dict):
            x_min = json_data.get('xMin')
            x_max = json_data.get('xMax')
            y_min = json_data.get('yMin')
            y_max = json_data.get('yMax')
            z_min = json_data.get('zMin', 0)
            z_max = json_data.get('zMax', 0)
            return cls(x_min, x_max, y_min, y_max, z_min, z_max)
        else:
            raise ValueError('json_data must be a dictionary')

    @staticmethod
    def list_from_command_result(result):
        if 'boundingBoxes2D' in result.keys():
            key = 'boundingBox2D'
            bounding_boxes = result['boundingBoxes2D']
        else:
            key = 'boundingBox3D'
            bounding_boxes = result['boundingBoxes3D']
        return [BoundingBox.FromDictionary(data.get(key, {})) for data in bounding_boxes]

    def __str__(self):
        if self.is_2D:
            return '<{}2D : ({:.2f} x {:.2f})>'.format(self.__class__.__name__, self.length, self.breadth)
        else:
            return '<{}3D : ({:.2f} x {:.2f} x {:.2f})>'.format(self.__class__.__name__, self.length, self.breadth, self.height)
