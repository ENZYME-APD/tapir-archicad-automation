#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

__all__ = ['Handshake_Component']

# - - - - - - - - LOCAL IMPORTS
from tapir_py import core

# - - - - - - - - RH/GH IMPORTS
from ghpythonlib.componentbase import dotnetcompiledcomponent as component
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML
import Grasshopper
import System

# - - - - - - - - COMPONENT
class Handshake_Component(component):

    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "Handshake", "Handshake_Component", """Establishes connection between current Grasshopper document, and any open ArchiCAD project.""", "tAPIr", "Connect")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("bb340f32-83f5-4140-9956-960003773db4")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_Boolean()
        self.SetUpParam(p, "Refresh", "Refresh", "Script variable Python")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
    
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Link", "ArchiCAD", "Script output ArchiCAD.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAFiUAABYlAUlSJPAAAADJSURBVEhL7ZNbDsQgCEV1/4vu8PYCpm0yyfyMJzFeEARMOw7/whyXKWOifU1avKtJSDzGhJY4i08UR0oAzN8KRNzylaaTkS9KuP20M6jZkEttpQl0ZC2mfiu8niLFK6m51um7CeCtd/GgXxfY+klvJrgvEPazX3PBL8XW+W+QLiZ+v9hB1WGX7911/2+IZNiYjmp8xqXrTqTcgJ0wQbwjLUvAKa0A6gXmFmBkPOw+LCBg17sJrDtSftgTcqeioSHGNe++DoevGeMDe+h/wjhtikQAAAAASUVORK5CYII="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.link = None
        self.archicad = None
    
    def RunScript(self, Refresh):
        self.archicad = core.Command.create()
        self.Message = "Port: {}".format(self.archicad.link.address)
        
        if not self.archicad.IsAlive():
            self.AddRuntimeMessage(RML.Warning, "Connection Failed: Unable to connect to ArchiCAD.")
        
        return self.archicad