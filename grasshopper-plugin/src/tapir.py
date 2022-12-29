#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['Plugin', 'RhinoWrapper']

# - - - - - - - - LOCAL IMPORTS
from tapir_py import parts, utility

# - - - - - - - - RH/GH IMPORTS
import System
import Rhino, Grasshopper, GhPython

# - - - - - - - - GLOBAL VARIABLES
Plugin = utility.RuntimeHelper({ "is_active" : False, "Archicad" : None }, "TapirPlugin")

# - - - - - - - - CLASS LIBRARY
class AssemblyInfo(GhPython.Assemblies.PythonAssemblyInfo):
    
    def get_AssemblyName(self):
        return "tAPIr"
    
    def get_AssemblyDescription(self):
        return """Grasshopper components to access ArchiCAD via JSON API."""

    def get_AssemblyVersion(self):
        return "0.1.0"

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
