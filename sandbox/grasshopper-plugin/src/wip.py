#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

__all__ = ["ExportCode_Component",
           "StealIcon_Component",
           "ClassificationItem_Component"]

# - - - - - - - - BUILT-IN IMPORTS
import System
import os, traceback

# - - - - - - - - LOCAL IMPORTS
import tapir, tapir_py

# - - - - - - - - RH/GH IMPORTS
from  ghpythonlib.componentbase import dotnetcompiledcomponent as component
from ghpythonlib.treehelpers import list_to_tree
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML
import Rhino, Grasshopper, GhPython
        
# - - - - - - - - COMPONENT
class ExportCode_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "ExportCode", "ExportCode", """Extracts code from a GhPython Script component, and provides option to export to .py file.""", "tAPIr", "00 WIP")
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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAKCSURBVHgB3ZWxbxJxFMff73pWht6FmJCQXmKOBY4cFRzExEUC1c3YzUUL/4HdLMWhThonh1ZXIc5yHY09pGMPE2oq5UiHdmh1IRFhtffz/a45ShSOa3Cxn4Tc7/fu3ff9fu+9PAD+d8ioF9Fo7LEFkOMAZArgR8cO7g/BIqut1u4GTIKizN1OZ+7S7W2Dvi9rlNHtdunHTZ0mb976IcsJv1ctboQ9lEzewEAKGEbNNgiCAPOZNNrCfp/vVwI8MjW4kWXZHwxefQBgpQRRTFz2TYMkzYIoiCCKgu2jaRtwdPS9EwgEg4HAlU673f7pFoB3FnhamRC+fn/hXv/6hvHZfkqzkh3IYf5OaknAoLpeYelMmebuFowjEo2Vi8USHcdiNmfXhvEW/SORWNlNlxtYpDKYYzeOj7/ZPyddrCaEo6716KeItaIkSf0Xa+uvoVR6B71er29jhV589NAuPuPUn8ieArA+RzE/EymiMMuvvvnBFh1Ft9tjB+u4BeAGbrBjGIa9rlR0yC8/cRVn6JUKNhxUwcsNgJLV5y9eVptmy87zIGxf1rS/PmYta1nWM/AS4LTVpkLra2+yGC2HJvnMjZ6dGlPX3GtphMAObrf29/fYE+uiHphmIwReUKKxT04rOuRXCva4WM4XKIplh3xDh2kNHRXU4rSVwlMwarW+LZ1OQ9M0oYY2Qqar4JGR01RRrmUJZy3NzAgJ1vesY7DLqtQir4ZNU3YDs/mVwHlhk5ONkXETdFSKJiISUevhsBofDKCqahzHTt3x4WACeB7/kDjQnCBM/ATrx3OXco7P+XP2B7boCWhYeRnlDqc4fqHRqH+BfwkLokTnDlT1ehwuHL8BoJMgDHftTCsAAAAASUVORK5CYII="
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

class StealIcon_Component(component):
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "StealIcon", "StealIcon", """Extracts 24x24 icon from input component.""", "tAPIr", "00 WIP")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("503f8223-189d-4a84-8e32-6ebf10afeab6")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_String()
        self.SetUpParam(p, "file", "source", "Script variable Python")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "stream", "stream", "Script output stream.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAIOSURBVHgB3VW/T9tAFH4X+cxCoswNg2mp1DBlqenY4mQm6V6ZuQW1WyEg8R/wQyHMiRgQDBB2AqwwZYIx+Q9AZMJn5fHOcHF+YZwwwSed/PTO/r6753ffAbx1sKBJRIwLIWxk7DtjLEUJ42mqhgANhljRdb0Mo8B13TlHiBsaGDTuHafuOI49FDl9uPEScZ+QEGuhyOWLw5J3jI1ePtZTFruFWJJxs9n0ctFoFIZBhLGspmnHAwUc162rH/ljNgPTyS+wuPiHRGKQSHyAkLjlmjZJTXHbJbC+vmWPR8dL1eopXF1ft3egIHcynUyCZc1CJmORYOJ5CcR/1F1bXQLGp6kjQJaFkPiZy3q7GyRELVwZ4zwn40g722JGEKHcgVy9wuFRxStjoVDse5fOR0rFvgCDVAA/fJsxYXVluS9f2C7C/6V8j4K/WL9EHz8jvALz9i/I5/0F6Jx73JEO2QaMANVdpfIuXFxcqnRNBR0latVgCJjmV9gpFuDs9AQWFn57uZNq1XtKn2rTqoBK9JcemxCwUtM0YYaI0+k0xGLdB/DurunnEOeVCfoChhGHCK9TGM9RC6YtC5J00CQmJgJ6vgfkwI0xXZ8cOGkYU3N7+wf4Ci/CF51VjOCkaoiwjipGcFQxwEkDIZ1Vml8I8ht5OT3Hw8II0Y+THmVQ+z2edsbkdVmj/DnnvKyc833iAbpECjpFWScMAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def set_icon(self, bitmap):
        self.SetIconOverride(bitmap)
        memory_stream = System.IO.MemoryStream()
        bitmap.Save(memory_stream, System.Drawing.Imaging.ImageFormat.Png)
        stream = System.Convert.ToBase64String(memory_stream.ToArray())
        return stream
    
    def RunScript(self, source):
        
        sources = self.Params.Input[0].Sources
        
        if source and os.path.isfile(source):
            bitmap = System.Drawing.Bitmap(source)
            return self.set_icon(bitmap)
        
        elif sources:
            source = sources[0].Attributes.GetTopLevel.DocObject
            bitmap = source.Icon_24x24
            return self.set_icon(bitmap)
        
        else:
            bitmap = None
            self.set_icon(self.get_Internal_Icon_24x24())
            self.AddRuntimeMessage(RML.Warning, "No Input")
            return 

class ClassificationItem_Component(component):
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "ClassificationItem", "ClassificationItem", """ClassificationItem""", "tAPIr", "00 WIP")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("608cd6d6-01be-4ef2-87cb-8760dcf4c859")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "classification", "classification", "Script variable Python")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.tree
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "children", "children", "Script output children.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAALEgAACxIB0t1+/AAAAZ1JREFUSEvdlT1PwlAUhvkJ/IQrgpsxcSVE/wGbEh0YDOpG1Dijzlp01Tj7sTCJ7OrcRHZbdqEzt02u5zSn3NIe+uHIm7xJc3rPc+5XTwvLLaVUUUrZnrpuT3qeJV1XkU0/JmWThuaX53l1ADkhKOuplFbuQpDYjYLSDCvqUHqycCAHyOguYXjBtjSDwePJxHcoOZNxawkXV/ggq7Vt1To8Vt/DobLsUQyUYAcvByG1DOOuef/wqHYb+2p9Y1OJUmXOGMN3OMaybQ6sDTePsFpitdyLQpN8ena+sBBeYcJqiZWKyYEC4woOWkex+I1xGy8CV5ewWtHEqPE8cMbcuxNYTbQIYbW4xDy+uLxKK7BmcYlprta2Zs8fn19BAZOwWnkPeaexp/rvAx94bXT9WIdWwR9yqdIOA6LGmeJeP7+8qt9x/AOci3G9SQhRBJCDMAS99Qfqx7J9h0FpxuZHyLiEKNefYIZcYmandVb3H500MORm66g4kAMkGSdG6dnkd9b5v9giO4kdNE1YCGbXA5szKBTGGDY1tnMukQqFP73q7HDtm/LxAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    
    def RunScript(self, classification):    
        items = []
        tree = classification
        archicad = tapir.Plugin.Archicad
        
        for branch_index in range(tree.BranchCount):
            branch = tree.Branch(branch_index)
            branch_item = []
            for leaf in branch:
                
                if isinstance(leaf, tapir_py.core.Command):
                    objects = leaf.GetAllClassificationSystems()
                    branch_item.append(objects)
                
                elif isinstance(leaf, tapir_py.parts.ClassificationSystem):
                    objects = archicad.GetAllClassificationsInSystem(leaf)
                    branch_item.append(objects)
            
                elif isinstance(leaf, tapir_py.parts.ClassificationItem):
                    objects = leaf.children
                    branch_item.append(objects)
            
            items.append(branch_item)
        
        items = list_to_tree(items)
        children = items
        
        # return outputs if you have them; here I try it for you:
        return children
