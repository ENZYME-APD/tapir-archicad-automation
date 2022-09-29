#!/usr/bin/env python27
# -*- coding: utf-8 -*-

__all__ = ['GetElementsOfType_Component']

# - - - - - - - - BUILT-IN IMPORTS
import json

# - - - - - - - - LOCAL IMPORTS
import tapir_py

# - - - - - - - - RH/GH IMPORTS
from ghpythonlib.componentbase import dotnetcompiledcomponent as component
from Grasshopper.Kernel import GH_RuntimeMessageLevel as RML
import Grasshopper
import System

# - - - - - - - - COMPONENT
class ElementInfo_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "Get Elements of Type", "getElementsOfType", """Collect elements of chossen type""", "tAPIr", "Selection")
        return instance
    
    def get_ComponentGuid(self):
        # _guid = tapir_py.tools.SwissKnife.component_guid(10)
        # return System.Guid(str(_guid))
        return System.Guid("fa831bc3-fc4a-43b5-9b22-5af3107a4002")
        # return System.Guid("fa831bc3-fc4a-43b5-9b22-9184e72a001L")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_Integer()
        self.SetUpParam(p, "ElementType", "ElementType", "Choose from list of element types")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p1 = Grasshopper.Kernel.Parameters.Param_GenericObject()
        p2 = Grasshopper.Kernel.Parameters.Param_GenericObject()
        
        self.SetUpParam(p1, "PrintToPanel", "PrintToPanel", "Prints list of avaliable types")
        self.Params.Output.Add(p1)

        self.SetUpParam(p2, "Elements", "Elements", "list of avaliable types")
        self.Params.Output.Add(p2)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result[0], DA, 0, True)
            self.marshal.SetOutput(result[1], DA, 1, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAJOgAACToAYJjBRwAAAGaSURBVEhL3ZRNSwJRFIYNonWbmbX+hSKoyFq0adOiNiEU1PwLNwkl/YH29kEwtCh0NgWBhkWRtWoRRAkFJRF96FCYX72d2z2KqYwjThsfeBjm3HPuqzPDdXUs3aTaomLGFr3kHvncogekQjZlRFEURCIRxGIxRKPRivFDaXVNaBgGVFUFzc7KLazxejwe5HI51JLKSmsRvWKGZhfkFtZ43W43TNPkccn1KzCuS5PvXGREr5ih2Xm5hTUNA+7SwPQuMGcATx9cZBwJEHzmgeI331ThSEC+BNy8AeuXwC1dq2k7IF8Elo6BsS2gfw04feAFxpF/IH79xDYwvAmcPXKRcSTApK92agcY+q+AzNc/B2QLwEwYGNwAEikuMm0HlOjTvM/Id9BHL3k/Kb+qMm0HpOnxrF4AgSNgMQ6snABXL7xItBwgzpVCgZ6JTUQvn0W2AkbFaRoKhaDr+h/FCRsOh+vqolfM0Kwmt7BmgBTNdQaDQfj9/oZr7CTZlB4yQCbIc77+qmlawufzVe7Zcs8yKWZt09WiHYnL9QMms/gCrBIbcAAAAABJRU5ErkJggg=="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def RunScript(self, ElementType):
        self.Message = '0.1.1'

        if ElementType is not None:
            types = tapir_py.parts.Element._TYPES
            _ElementType = tapir_py.tools.SwissKnife.get_wrapped_value(ElementType,types)
            PrintToPanel  = tapir_py.tools.SwissKnife.show_menu('Element Types',_ElementType,types)

            _link_from_sticky  = tapir_py.tools.SwissKnife.load_link()
            _Elements = tapir_py.core.Command.create(_link_from_sticky.port).GetElementsByType(types[_ElementType])
            Elements =[e.guid for e in _Elements]
            return PrintToPanel,Elements
            
            
        else:
            self.AddRuntimeMessage(RML.Warning, 'No Input!')