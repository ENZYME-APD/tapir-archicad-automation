#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ["Plugin", "RhinoWrapper", "Factory"]

# - - - - - - - - BUILT-IN IMPORTS
import System, traceback
import base64

# - - - - - - - - LOCAL IMPORTS
from tapir_py import parts, utility

# - - - - - - - - RH/GH IMPORTS
import Rhino, Grasshopper, GhPython

# - - - - - - - - GLOBAL VARIABLES
Plugin = utility.RuntimeObject( { "is_active" : False, "Archicad" : None }, "TapirPlugin")

# - - - - - - - - DECORATORS
def connect(function):
    def wrapper(*args, **kwargs):
        if Plugin.is_active:
            return function(*args, **kwargs)
        else:
            args[0].AddRuntimeMessage(Grasshopper.Kernel.GH_RuntimeMessageLevel.Warning, "Plugin Disconnected!")
    return wrapper

def debug(function):    
    def wrapper(*args, **kwargs):
        try:
            return function(*args, **kwargs)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), str(ex))
            args[0].AddRuntimeMessage(Grasshopper.Kernel.GH_RuntimeMessageLevel.Error, str(ex))
        return wrapper

# - - - - - - - - CLASS LIBRARY
class AssemblyInfo(GhPython.Assemblies.PythonAssemblyInfo):
    
    def get_AssemblyName(self):
        return "tAPIr"
    
    def get_AssemblyDescription(self):
        return """Grasshopper components to access ArchiCAD via JSON API."""

    def get_AssemblyVersion(self):
        return "1.1.0"

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
    def create_button(component, param_index, name, x_offset=27, y_offset=11):
        source_input = component.Params.Input[param_index]
        component_width_delta = component.Attributes.Bounds.Width * 0.5

        # Create new Button component
        source_pivot = component.Attributes.Pivot
        new_button = Grasshopper.Kernel.Special.GH_ButtonObject()

        # Add component to current Grasshopper Document
        doc = component.OnPingDocument()
        doc.AddObject(new_button, False, doc.ObjectCount + 1)

        new_button.Name = name
        new_button.NickName = name
        new_button.Attributes.Pivot = System.Drawing.PointF(source_pivot.X - component_width_delta - x_offset, source_pivot.Y - y_offset)

        # Connect Value List to component
        source_input.AddSource(new_button)
        new_button.ExpireSolution(True)

        return new_button

    @staticmethod
    def create_value_list(component, param_index, name, items, x_offset=27, y_offset=11):
        source_input = component.Params.Input[param_index]
        component_width_delta = component.Attributes.Bounds.Width * 0.5

        # Create new Value List component
        source_pivot = component.Attributes.Pivot
        new_value_list = Grasshopper.Kernel.Special.GH_ValueList()
        
        # Add component to current Grasshopper Document
        doc = component.OnPingDocument()
        doc.AddObject(new_value_list, False, doc.ObjectCount + 1)

        new_value_list.Name = name
        new_value_list.NickName = name
        new_value_list.Attributes.Pivot = System.Drawing.PointF(source_pivot.X - component_width_delta - x_offset, source_pivot.Y - y_offset)
        new_value_list.ListItems.Clear()

        # Populate list
        Factory.update_value_list(new_value_list, items)

        # Connect Value List to component
        source_input.AddSource(new_value_list)
        new_value_list.ExpireSolution(True)

        return new_value_list

    @staticmethod
    def update_value_list(component, items):
        if isinstance(component, Grasshopper.Kernel.Special.GH_ValueList):
            component.ListItems.Clear()
            for item in items:
                list_item = Grasshopper.Kernel.Special.GH_ValueListItem(str(item), '"{}"'.format(item))
                component.ListItems.Add(list_item)
            return True
        else:
            return False

# ----- UTILLITIES
def ico2base64(ico_file):
    """
    Converts icon image to base64 string 
    to be used by get_internal_Icon_24x24 method
    """
    with open(ico_file, "rb") as image_file:
        encoded_string = base64.b64encode(image_file.read())
        image_file.close()
    return encoded_string
