#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['ElementInfo_Component']

# - - - - - - - - BUILT-IN IMPORTS
import json

# - - - - - - - - RH/GH IMPORTS
from ghpythonlib.componentbase import dotnetcompiledcomponent as component
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML
import Grasshopper
import System

# - - - - - - - - COMPONENT
class ElementInfo_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "Element Info", "ElementInfo", """ElementInfo""", "tAPIr", "BimX")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("fa831bc3-fc4a-43b5-9b22-31973283d01c")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_String()
        self.SetUpParam(p, "URL", "URL", "Script input URL.")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "ElementInfo", "ElementInfo", "Script output ElementInfo.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAJOgAACToAYJjBRwAAAGaSURBVEhL3ZRNSwJRFIYNonWbmbX+hSKoyFq0adOiNiEU1PwLNwkl/YH29kEwtCh0NgWBhkWRtWoRRAkFJRF96FCYX72d2z2KqYwjThsfeBjm3HPuqzPDdXUs3aTaomLGFr3kHvncogekQjZlRFEURCIRxGIxRKPRivFDaXVNaBgGVFUFzc7KLazxejwe5HI51JLKSmsRvWKGZhfkFtZ43W43TNPkccn1KzCuS5PvXGREr5ih2Xm5hTUNA+7SwPQuMGcATx9cZBwJEHzmgeI331ThSEC+BNy8AeuXwC1dq2k7IF8Elo6BsS2gfw04feAFxpF/IH79xDYwvAmcPXKRcSTApK92agcY+q+AzNc/B2QLwEwYGNwAEikuMm0HlOjTvM/Id9BHL3k/Kb+qMm0HpOnxrF4AgSNgMQ6snABXL7xItBwgzpVCgZ6JTUQvn0W2AkbFaRoKhaDr+h/FCRsOh+vqolfM0Kwmt7BmgBTNdQaDQfj9/oZr7CTZlB4yQCbIc77+qmlawufzVe7Zcs8yKWZt09WiHYnL9QMms/gCrBIbcAAAAABJRU5ErkJggg=="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def RunScript(self, URL):
        
        # Updated so we can differentiate between old and new component.
        self.Message = '0.1.1'

        if URL:
            element = { 'url' : URL}
            return json.dumps(element, indent=4)
        
        else:
            self.AddRuntimeMessage(RML.Warning, 'No Input!')