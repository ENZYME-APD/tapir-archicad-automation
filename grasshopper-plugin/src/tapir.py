#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['Plugin', 'RhinoWrapper']

# - - - - - - - - BUILT-IN IMPORTS
import traceback, time
import System

# - - - - - - - - LOCAL IMPORTS
from tapir_py import parts, utility

# - - - - - - - - RH/GH IMPORTS
import Rhino, Grasshopper, GhPython

# - - - - - - - - GLOBAL VARIABLES
Plugin = utility.RuntimeHelper({ "is_active" : False, "Archicad" : None }, "TapirPlugin")

# - - - - - - - - DECORATORS
def connect(function):
    def wrapper(*args, **kwargs):
        if Plugin.is_active:
            return function(*args, **kwargs)
        else:
            args[0].AddRuntimeMessage(Grasshopper.Kernel.GH_RuntimeMessageLevel.Warning, "Plugin Disconnected!")
    return wrapper

# - - - - - - - - CLASS LIBRARY
class AssemblyInfo(GhPython.Assemblies.PythonAssemblyInfo):
    
    def get_AssemblyName(self):
        return "tAPIr"
    
    def get_AssemblyDescription(self):
        return """Grasshopper components to access ArchiCAD via JSON API."""

    def get_AssemblyVersion(self):
        return "1.0.0"

    def get_AuthorName(self):
        return "EnzymeAPD"
    
    def get_Id(self):
        return System.Guid("54b11162-627b-455b-b9c0-963e76b36dc7")

class RhinoWrapper(object):

    @staticmethod
    def BoundingBox(boundingBoxes):

        success = True
        message = ""
        rhino_boxes = []
        for bBox in list(boundingBoxes):

            if isinstance(bBox, parts.BoundingBox):
                X = Rhino.Geometry.Interval(*bBox.x)
                Y = Rhino.Geometry.Interval(*bBox.y)
                Z = Rhino.Geometry.Interval(*bBox.z)
            
                box = Rhino.Geometry.Box(Rhino.Geometry.Plane.WorldXY, X, Y, Z)
                rhino_boxes.append(box)
            else:
                success = False
                message = "Some inputs were skipped."

        return success, message, rhino_boxes

class Factory(object):

    @staticmethod
    def create_value_list(component, param_index, name, items, x_offset = 27, y_offset = 11):
        source_input = component.Params.Input[param_index]

        # Create new Value List component
        source_pivot = source_input.Attributes.Pivot

        new_value_list = Grasshopper.Kernel.Special.GH_ValueList()
        # Add component to current Grasshopper Document
        doc = component.OnPingDocument()
        doc.AddObject(new_value_list, False, doc.ObjectCount + 1)

        new_value_list.Name = name
        new_value_list.NickName = name[0].upper()
        new_value_list.Attributes.Pivot = System.Drawing.PointF(source_pivot.X - x_offset, source_pivot.Y - y_offset)
        new_value_list.ListItems.Clear()

        # Populate list
        Factory.update_value_list(new_value_list, items)

        # Connect Value List to component
        source_input.AddSource(new_value_list)

    @staticmethod
    def update_value_list(component, items):
        if isinstance(component, Grasshopper.Kernel.Special.GH_ValueList):
            component.ListItems.Clear()
            for item in items:
                list_item = Grasshopper.Kernel.Special.GH_ValueListItem(str(item), item)
                component.ListItems.Add(list_item)
            return True
        else:
            return False
