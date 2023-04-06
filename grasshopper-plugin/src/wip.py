#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

__all__ = ['ExportCode_Component']

# - - - - - - - - BUILT-IN IMPORTS
import System
import traceback

# - - - - - - - - LOCAL IMPORTS
import tapir

# - - - - - - - - RH/GH IMPORTS
from  ghpythonlib.componentbase import dotnetcompiledcomponent as component
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML
import Rhino, Grasshopper, GhPython

# - - - - - - - - COMPONENT
class ExportCode_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "ExportCode", "ExportCode", """Extracts code from a GhPython Script component, and provides option to export to .py file.""", "tAPIr", "99 WIP")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("d2a22074-0167-4365-a93b-924680662805")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True

    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "GhPython", "G", "GhPython Script component.")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Code", "C", "Code as multiline text.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAALEgAACxIB0t1+/AAAAntJREFUSEutVbuO00AUzXrGjtmM42QlQBQU60RiG2yDRJMiDhAnUQqoEB01PUICiW9ArBDN8gkUFBQ0IMQn8AEglohQEVYCGt7nGt9oPPFqvYLiaHxf59w7M7Zrnuc11yxrG/gE/KwKIeWFyWQibMeOYL8GduFLyMeYTqdWDYF7wO/DQtr2xdFoJIUQtzT/DP7z5CeMx2NBAgstoTJIAATSEuKOEftoO84ZEqApSOA7BxtKnYjj2Imi0AnDMFujKMqfIyeOo9w+7SRJX6ZpWiaQieSTFAW63a7L41ExP1OnZNPa7/el67oB9vsKyG+i7hnXG5ghZ1AQ6HSCTIDJmJxXjH4WeU+Aqtv6viAQBIFbRu77/ga63UHOD86tiA/GBJ3lFhE5ifmt1gZizzknx65lWY8hehfPL4wYY44tGpYKcOebwaa7JsQjjgOvUHTZazaPDgaDLGefQ15ISf2lxUOmLWLyvPgG/L+APdz366e2tuocI1BuicACZ3WO4ivXlCbgM2g0Gsfhozd0D12nOjGDciFwm+uBOXXOcQiFBYFut7OcQEhxDb5vIL/KBTq4kfxmzYA57n46HA4zv1Ovh/C9MSb4e00pAfZTOsRer2frxBRjcraTJLHp/WA7F31HnCtnQElEAPvlkfX1k0zMoBg/m2IcR+1b5tz3DLA1l7hIL2ayspXjOmfpBCb0YrbNleKcr3NWEtCLTTEzTtA5DxSo0rkuRtA5CwJ0TTnJLC5bzc4ZOqchcPDnuixuQucsCChPHaOEKp3rYjra7XYLXPTfXgro33b67qz84A8J4mC+ryRwX3P8bzysKaV8S1gPYHw2gv+CL8COUsr5AzEo6O3WaD7oAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.code = ""
    
    def RunScript(self, component):
        self.code = ""
        if self.Params.Input.Count == 0 or self.Params.Input[0].Sources.Count == 0:
            self.AddRuntimeMessage(RML.Warning, "Connect a GhPython Script component.")
            
        else:
            input_sources = self.Params.Input[0].Sources
            target_component = None
            
            if input_sources[0].Attributes.IsTopLevel:
                target_component = input_sources[0]
            else:
                target_component = input_sources[0].Attributes.GetTopLevel.DocObject
            
            if isinstance(target_component, GhPython.Component.ZuiPythonComponent):
                self.code = target_component.Code
            else:
                self.AddRuntimeMessage(RML.Error, "Input is not a GhPython Script component.")
        
        return self.code
    
    def OnExportClick(self, sender, args):
        file_dialog = Rhino.UI.SaveFileDialog()
        file_dialog.Title = "Export Python FIle"
        file_dialog.Filter = "Python files (*.py)|*.py";
        file_dialog.ShowDialog()

        if file_dialog.FileName != "":
            with open(file_dialog.FileName, 'wt') as file:
                file.write(self.code)
            Rhino.RhinoApp.WriteLine("File Exported Successfully!\n{}".format(file_dialog.FileName))
    
    def AppendAdditionalMenuItems(self, items):
        component.AppendAdditionalMenuItems(self , items)
        if self.code == "":
            return
        try:
            export_menu_item = items.Items.Add('Export Python FIle', None, self.OnExportClick)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)
