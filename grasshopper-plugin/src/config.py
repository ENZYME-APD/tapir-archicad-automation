#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['AssemblyInfo']

# - - - - - - - - RH/GH IMPORTS
import System
import GhPython

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