#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

# NOTE: This is Optional
# __all__ is a list of strings, containsing the class / component names.
__all__ = ['Sample_Component']

# - - - - - - - - BUILT-IN IMPORTS
import System
import traceback # for error handling

# NOTE: tapir_py is copied just before compiling the grasshopper plugin.
# So, it is assumed that tapir_py can be imported locally.
# - - - - - - - - LOCAL IMPORTS
import tapir
from tapir_py import core

# - - - - - - - - RH/GH IMPORTS
# All components / nodes inherit from 'dotnetcompiledcomponent' class.
from ghpythonlib.componentbase import dotnetcompiledcomponent as component
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML # for error handling
import Rhino, Grasshopper, GhPython

# - - - - - - - - COMPONENT
class Sample_Component(component):

    # Runs when the grasshopper window is loaded
    def __new__(cls):
        # Creates a new instance of this class / component inside Grasshopper.
        # Source: https://developer.rhino3d.com/api/grasshopper/html/T_Grasshopper_Kernel_GH_Component.htm
        # cls: Component Class
        # Name
        # NickName
        # Description
        # Category (The plugin category you see on Grasshopper Ribbon)
        # SubCategory
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
                                                            "ComponentName",
                                                            "ComponentNickName",
                                                            "Component description goes here.",
                                                            "tAPIr", # NOTE: Do not change this
                                                            "SubCategory")
        return instance
    
    def get_ComponentGuid(self):
        # It is better to generate a GUID and then paste it here.
        # guid = System.Guid.NewGuid()
        # But this GUID needs to be static, and should be unique
        # to each component or node.
        # So when changes are made to this component, old components
        # are updated by matching this GUID.
        return System.Guid("00000000-0000-0000-0000-000000000000")
    
    # NOTE: DO Not change this.
    # This is a template method to create input(s) / output(s) on the component.
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True

    # At this point, there are no input / output parameters on this component.
    # Create and Register Inputs to the component
    def RegisterInputParams(self, pManager):
        """
        Different type of parameters can be set.

        Param_Arc
        Param_Boolean
        Param_Box
        Param_Brep
        Param_Circle
        Param_Colour
        Param_Complex
        Param_Culture
        Param_Curve
        Param_Field
        Param_FilePath
        Param_GenericObject
        Param_Geometry
        Param_Group
        Param_Guid
        Param_Integer
        Param_Interval
        Param_Interval2D
        Param_Interval2D_OBSOLETE
        Param_LatLonLocation
        Param_Line
        Param_Matrix
        Param_Mesh
        Param_MeshFace
        Param_MeshParameters
        Param_Number
        Param_OGLShader
        Param_Plane
        Param_Point
        Param_Rectangle
        Param_ScriptVariable
        Param_String
        Param_StructurePath
        Param_SubD
        Param_Surface
        Param_Time
        Param_Transform
        Param_Vector
        """
        # Define the type of input paramter.
        new_parameter = Grasshopper.Kernel.Parameters.Param_Point()
        self.SetUpParam(new_parameter,
                        name="Position",
                        nickname="P",
                        description="Position as Point")
        # This sets up the type of access (item / List / Tree)
        new_parameter.Access = Grasshopper.Kernel.GH_ParamAccess.item
        
        # Add this paramter to this component under Inputs
        self.Params.Input.Add(new_parameter)

    def RegisterOutputParams(self, pManager):
        # Define the type of output parameter
        new_parameter = Grasshopper.Kernel.Parameters.Param_Vector()
        self.SetUpParam(new_parameter,
                        name="Vector",
                        nickname="V",
                        description="Position Vector")
        # Add this parameter to this component under Outputs
        self.Params.Output.Add(new_parameter)

        # Repeat same if you want to add more outputs
        second_parameter = Grasshopper.Kernel.Parameters.Param_Line()
        self.SetUpParam(second_parameter,
                        name="Line"
                        nickname="L",
                        description="Line representing the Position Vector")
        self.Params.Output.Add(second_parameter)
    
    # This is the method that the component calls
    # to execute your script.
    # It interacts directly with IGH_DataAccess class (input / output data)
    # It calls the RunScript method where you can place the business logic.
    def SolveInstance(self, DA):
        # number of inputs need to match the number of inputs registered.
        p0 = self.marshal.GetInput(DA, 0)
        #p1 = self.marshal.GetInput(DA, 1) # If there are two inputs
        result = self.RunScript(p0) #result = self.RunScript(01, 01) # If there are two inputs

        # Pass result / return values from RunScript to output params
        if result is not None:
            self.marshal.SetOutput(result[0], DA, 0, True) # Vector - Check RunScript
            self.nmarshal.SetOutput(result[1], DA, 1, True)  # Line - Check RunScript

    # Set the Icon for this component.
    # NOTE: Creating Icon as a 64Base String is outside the scope of this tutorial. I will skip it for now.
    def get_internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAFiUAABYlAUlSJPAAAADJSURBVEhL7ZNbDsQgCEV1/4vu8PYCpm0yyfyMJzFeEARMOw7/whyXKWOifU1avKtJSDzGhJY4i08UR0oAzN8KRNzylaaTkS9KuP20M6jZkEttpQl0ZC2mfiu8niLFK6m51um7CeCtd/GgXxfY+klvJrgvEPazX3PBL8XW+W+QLiZ+v9hB1WGX7911/2+IZNiYjmp8xqXrTqTcgJ0wQbwjLUvAKa0A6gXmFmBkPOw+LCBg17sJrDtSftgTcqeioSHGNe++DoevGeMDe+h/wjhtikQAAAAASUVORK5CYII="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))
    
    # - - - - - - - - - - - - - - - - - - - - - - - -
    
    def __init__(self):
        # Any initialisations can be done here
        pass

    # NOTE: The number of arguments for Runscript
    # should match the number of inputs that were
    # created in RegisterInputParams()
    def RunScript(self, point):
        # This is where the your code needs to be placed.
        # Everything inside RunScript is executed everytime
        # the component is refreshed / recomputed,
        # or when inputs / outputs are connected and 
        # disconnected.

        # SAMPLE CODE
        origin = Rhino.Geometry.Point3d.Origin
        line = Rhino.Geometry.Line(origin, point)
        vector = line.Direction

        # The number and order of Returning values
        # need to match the number and order of outputs
        # that were created in RegisterOutputParams()
        return vector, line
      
