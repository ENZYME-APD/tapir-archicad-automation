#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

__all__ = ["GetElements_Component",
           "GetAllClassificationSystems_Component_OBSOLETE",
           "GetClassificationSystems_Component",
           "GetElementsByClassification_Component",
           "GetBoundingBox_Component",
           "HighlightElements_Component"]

# - - - - - - - - BUILT-IN IMPORTS
import System
import traceback

# - - - - - - - - LOCAL IMPORTS
import tapir

# - - - - - - - - RH/GH IMPORTS
from ghpythonlib.componentbase import dotnetcompiledcomponent as component
from ghpythonlib.treehelpers import list_to_tree
import Grasshopper

# - - - - - - - - COMPONENTS
class GetElements_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "GetElements", "GetElements", """Retrieves Element information from connected ArchiCAD project.""", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("4a16caba-b37f-4f7f-80c1-cd23cb4a0841")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_Boolean()
        self.SetUpParam(p, "Update", "U", "Updates output Elements list.")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        p.SetPersistentData(Grasshopper.Kernel.Types.GH_Boolean(True))
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Elements", "E", "List of Elements from ArchiCAD project.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAH8SURBVHgB3VUxT8JAFH7XdmCQhsG4sJyLtCmGuqhxVuOk3RXlH7iqizpo3IxRZ0Hc1c1EF38BGq3gRIdCojFR28RgYjzvGokVrgTphN/S3nvv3nv9Xu87gG4Hqr8oSnKVrtYCIwmslUq36/UlxjgWifQsAkIZ6sRAiEXTZWs1aceyrl6a9itqkjiOQ3h4fXUI8/uTJxJaYXNzi9h2xYu5uyuSpeUVklCTBYz1GLdAK/gLsK9lyXjYoEU9Nr4hQEcgmYX5NNezMD9HmSKZcAUQwoqicF3xeJwF4HAFAFmVSpXrKZbuPX+4AoRkDw/zXJdnp/76WvI7XdeFaDTatInZ/YhEpJ18/shwXFdnnDO6KtUq5HJ5OD45vXqnv2o9tuNzoOt6rFb7+DkHjBbaeeA56FqIPKOqDm739vaNPj09XkJIoEYDHRgGJJVlWX55fHjrD8sn5zcVVw1jGtLpWSpmdIhtgmnTwICWahnEumeaY9u2J3DDI2PPXOHiQNO0lKJo5cYiAq97dtxlOfqnrzBN81oUwRAEOPEXQf7uGfcX52ffegLgOC5MTE41zYJ1yvQouByySsWb/l8muumAJ8G7e/u/5LcVPJrUwbKmDaUakv9wz7ts2plFUHJPixCSZthtMj4xFbSfzkKYoc9cUMDnJ8qKgmSYZuEa/hW+AG0WQhBYhGOoAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.state = 0
    
    def set_state(self, state):
        self.state = state
        self.update_icon()
        self.update_message()

    def update_message(self):
        if self.state == 0:
            self.Message = "Get Selected"
        elif self.state == 1:
            self.Message = "Get All"
        elif self.state == 2:
            self.Message = "Get By Classification"
        else:
            self.Message = ""

    def update_icon(self):
        if self.state == 0:
            o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAGYSURBVHgB3VRNTsJAFH5vOglNFMKCuB42UFIM3UnceQI5AjfAG4AnMNyAI3AD48ZEN0CkoTu6NiwIXVgT7DgVRqAMBcsKv1Xfz7zve/PmFeDUgfLDMMpNYbV2ZnJoOc7wXpqMsayunzcAsS6CDDh3RbmO79O26/an2wSlMn99eYZ0Or1Vezbz4Kp6Dc5oiLJ4KnX2iAStbR3Q//ygN5KErAdVxUNkMpv+ULmq+FKxpevzhrQJJAKvx4ZxFU9GgMj2JLDjCADdQ+PJCDjvHBrfIPA8T5kf9es6bYth9pW1hT98qtJOvAeWZWV9f77ag/BahPLoHpw+NJWzVLp8yOUuqpPJ+xMcCYw6DMNggHQshjUVK58/9j4Vz1RrLpmz6yu/D8Wi2SsUzEoswUJ9+CoW4Ah3jFlZOACUQp0Q6EZJiEq9xF+6sG17oGlQi5Lgpno6jh5UzcIwzHH8/whdZ/SWD7/oLvWKLn6XzHHsvCrXNM3KV0C6GqG1tfO71cd1EVfctnsD6f+ZASK9hRgsughic4IAO9Hi/wPfJCWfBKxXgIgAAAAASUVORK5CYII="
        elif self.state == 1:
            o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAH8SURBVHgB3VUxT8JAFH7XdmCQhsG4sJyLtCmGuqhxVuOk3RXlH7iqizpo3IxRZ0Hc1c1EF38BGq3gRIdCojFR28RgYjzvGokVrgTphN/S3nvv3nv9Xu87gG4Hqr8oSnKVrtYCIwmslUq36/UlxjgWifQsAkIZ6sRAiEXTZWs1aceyrl6a9itqkjiOQ3h4fXUI8/uTJxJaYXNzi9h2xYu5uyuSpeUVklCTBYz1GLdAK/gLsK9lyXjYoEU9Nr4hQEcgmYX5NNezMD9HmSKZcAUQwoqicF3xeJwF4HAFAFmVSpXrKZbuPX+4AoRkDw/zXJdnp/76WvI7XdeFaDTatInZ/YhEpJ18/shwXFdnnDO6KtUq5HJ5OD45vXqnv2o9tuNzoOt6rFb7+DkHjBbaeeA56FqIPKOqDm739vaNPj09XkJIoEYDHRgGJJVlWX55fHjrD8sn5zcVVw1jGtLpWSpmdIhtgmnTwICWahnEumeaY9u2J3DDI2PPXOHiQNO0lKJo5cYiAq97dtxlOfqnrzBN81oUwRAEOPEXQf7uGfcX52ffegLgOC5MTE41zYJ1yvQouByySsWb/l8muumAJ8G7e/u/5LcVPJrUwbKmDaUakv9wz7ts2plFUHJPixCSZthtMj4xFbSfzkKYoc9cUMDnJ8qKgmSYZuEa/hW+AG0WQhBYhGOoAAAAAElFTkSuQmCC"
        elif self.state == 2:
            o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAALEgAACxIB0t1+/AAAAa5JREFUSEvdVDtOw0AQzRFyBFfxrtYmpgCJGkQNos6HGpDbIAoqEF2OwOcAMT1BqZMmR0iTPlIusMzb7Dp2GCdxqMKTnuL1jt/Mm9ls5X9CiOBRymAG4tm+zsHzvCr2hAwnQgZaCDXB2vOiqg3hQYGti8srPZ/P9XQ61Xj2/SC22wYQ9301NsIr9GUwLkxCFceoZDgcmQTAV//bVqdaNsw45MRTcq7xEtVCEOKnZ+f67f1D95JPjYTYcx+ahJxwSjUxolmg32jJcDTSN7d3+un5xYgiIZL1egnszxDLi+ZpRLNAAtMO2jw6PtGd+wdDiONdn/aWCexgCxn+duBaBAeNZjsNbjSvjXi+RTvMABDioEUBA3KTSBkmFDhw6+xJiqKoSu/KnyJACOHREF9xDC273AdIsnDi2kW/tN74P1h8GLaztFt7BOp/t3BQf4WZwWJYs4393AUYcHoqSrjAgajVVN0uebjqHcu4UErVcYWsTZKr3rGEi7VJVqt35FxAhItdkr0qmOodt3RhHJC4Uod5B0XVO24zi0JxgO6VmBPOke4pG84CdxMrvueoVH4AzCJavwTgX24AAAAASUVORK5CYII="
        else:
            o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAIOSURBVHgB3VW/T9tAFH4X+cxCoswNg2mp1DBlqenY4mQm6V6ZuQW1WyEg8R/wQyHMiRgQDBB2AqwwZYIx+Q9AZMJn5fHOcHF+YZwwwSed/PTO/r6753ffAbx1sKBJRIwLIWxk7DtjLEUJ42mqhgANhljRdb0Mo8B13TlHiBsaGDTuHafuOI49FDl9uPEScZ+QEGuhyOWLw5J3jI1ePtZTFruFWJJxs9n0ctFoFIZBhLGspmnHAwUc162rH/ljNgPTyS+wuPiHRGKQSHyAkLjlmjZJTXHbJbC+vmWPR8dL1eopXF1ft3egIHcynUyCZc1CJmORYOJ5CcR/1F1bXQLGp6kjQJaFkPiZy3q7GyRELVwZ4zwn40g722JGEKHcgVy9wuFRxStjoVDse5fOR0rFvgCDVAA/fJsxYXVluS9f2C7C/6V8j4K/WL9EHz8jvALz9i/I5/0F6Jx73JEO2QaMANVdpfIuXFxcqnRNBR0latVgCJjmV9gpFuDs9AQWFn57uZNq1XtKn2rTqoBK9JcemxCwUtM0YYaI0+k0xGLdB/DurunnEOeVCfoChhGHCK9TGM9RC6YtC5J00CQmJgJ6vgfkwI0xXZ8cOGkYU3N7+wf4Ci/CF51VjOCkaoiwjipGcFQxwEkDIZ1Vml8I8ht5OT3Hw8II0Y+THmVQ+z2edsbkdVmj/DnnvKyc833iAbpECjpFWScMAAAAAElFTkSuQmCC"
        self.SetIconOverride(System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o))))

    @tapir.connect
    def RunScript(self, refresh):
        if self.state == 0:
            return tapir.Plugin.Archicad.GetSelectedElements()
        elif self.state == 1:
            return tapir.Plugin.Archicad.GetAllElements()
        elif self.state == 2:
            return tapir.Plugin.Archicad

    def on_get_selected_click(self, sender, args):
        try:
            # TODO: Add Update Input Parameter here.
            self.set_state(0)
            self.ExpireSolution(True)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

    def on_get_all_click(self, sender, args):
        try:
            # TODO: Remove Update input Parameter here.
            self.set_state(1)
            self.ExpireSolution(True)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

    def on_get_by_classification_system(self, sender, args):
        try:
            self.set_state(2)
            self.ExpireSilution(True)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

    def AppendAdditionalMenuItems(self , items):
        component.AppendAdditionalMenuItems(self , items)
        try:
            item_a = items.Items.Add('Get Selected', None, self.on_get_selected_click)
            item_b = items.Items.Add('Get All', None, self.on_get_all_click)
            item_c = items.Items.Add('Get By Classification System', None, self.on_get_by_classification_system)
            item_c.Enabled  = False
            
            if self.state == 0:
                item_a.Checked = True
                item_b.Checked = False
            
            elif self.state == 1:
                item_a.Checked = False
                item_b.Checked = True
            
            else:
                item_a.Checked = False
                item_b.Checked = False
            
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

class GetAllClassificationSystems_Component_OBSOLETE(component):

    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "GetAllClassificationSystems", "GetAllClassificationSystems", "Component description goes here.", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("25fb8ccf-7ced-41e2-b2d9-5e4acfd69e06")

    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True

    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_Boolean()
        self.SetUpParam(p, "Update", "U", "Updates output Classification Systems list.")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)

    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "ClassificationSystems", "CS", "ClassificationSystems come out here")
        self.Params.Output.Add(p)
    
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)

    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAJ4SURBVHgB1VXPaxNREJ7dLGQLbughPdh6aCvt7mYTU8EInoQmeM0WQb2o8Sa2mHizrdA/oS0qeJPUu4lHoUGv3QilmGaTXlIhPZlisvGQgtnxvU1/bEIa1xoRP1jeezPvzcz7Zt4swP8OppdSlv1xBCYBgKOAuIvIrBSLuVXoB2Q5sKzO3MSNDQ0p8nkd6VqWlWX4U0xM+KamwzfQMAy0o1YzMHT1GkpS4LpTW2yngB7mOHYpEpkGQRDadB6PAKoaJWxhYnJSCYIDuOwLSovgOffq9p1bkhqNwtCQF5Jrb+Di+Di43W5rj9frBX7ALVX29x/yPD9YqXx938vBcZJF0R+XfeLKWvJ1W+TzC4vgETwgeFqyudlH1mgYdbgfewB6vphwlHhJUkq6rh/z/fzFS1xfz2C5XEZJ9uPdezEr0Ue6t6m0VQCSrJTACaiRI5T39nB27rFlhMKuo8bDkZMCoLpedrmTKbOradnRFi3PoF6vW1TNkKTa6cpkPlhyTdNgeOSCdQ6cQBQvxWmd20vz6fwC+RatKOncrqcjXdPc9bLb9pJpFQ2PnE+Ew60SzWY/kUizrY0MUw2FrgySz7pdKvUOjNp38rI/P3HsgMLn802ZJnvIC/Ol0XCl+YEf3wp6jiEBqCT4IMOwVURzq1DIfYR+4FeJ7IVTm50k+ZcIL7HWijS7w2SiiV0bnigqm4gQ29nZ3rLLudMcNBrcKs83kp3ygwO+2m0/x0Gs2YQ0aSFqp5O+QVGUIH2w9j7FwRlAjRD6RjvlTRMs0lkXkyazsTM7KBS2x7rJ6Q2aJpt2sZwK/YZFjxwoKcplR238t0F+r5t/zfg/xU9Qkkw5GVlaDwAAAABJRU5ErkJggg=="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def get_Exposure(self):
        return Grasshopper.Kernel.GH_Exposure.hidden
    
    @tapir.connect
    def RunScript(self, refresh):
        return tapir.Plugin.Archicad.GetAllClassificationSystems()

class GetClassificationSystems_Component(component):

    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "GetClassificationSystems", "GetClassificationSystems", """Gets ClassificationSystems from current ArchiCAD project.""", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("9c083b2b-c5d1-4f68-b2e2-e247d7261747")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_String()
        self.SetUpParam(p, "classificationSystems", "classificationSystems", "Script variable Python")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "classificationSystems", "classificationSystems", "Script output classificationSystems.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAALEgAACxIB0t1+/AAAAbNJREFUSEvdlbFSwjAYx3kEH4EFkjQN4KB3TnriOeM5izjryQo6OFnZGNRdfQDxAeoD4MJG2VjK7NEXiPmHFCkq12I57/zd5drmS//J98+XNvO/YcyuUyaGlNmSUj4kxK6b0O9hTLQrB4ey232ToN/3JJ4Z420zZHlyOau0W96XQRBo8ZDxOJAbm1sqG7FthiYHLytrOs5Ny8hGuXZaUlnVyed50bwSH9iCFUIcloCHx6dIJuhHHBkmsgsbCI/nbWk0L6SjVn17d69bCOzC+NgbjyrxvMmqAcRc91X6vi9RRUfV2jQrxJ47L7oAKONDI7EYiIT4o5E8PTvXImA2BvHy3mcBIGYkFoN6x4rQ4C/2otG8nIoA2IV+xF3XlX1voGIiXgaEFL7sAQQxCSbA/Wwc10R7AFBFSB9VAiuqxydaHI1Z4h3P6EccmRAikh86y7JKlNpXkyZq2WxpLfRZLaCCfsYKdXXd0S+kQThBqugM9IfOfOzM/U+eE8J7iU63tobS7HxDvxkSgXNexFla6hMSl9QmgUhYXd+3mGcjKToDJc75evo2rVQcqH9Ib2Xif0gm8wGOuY8PG8oa8gAAAABJRU5ErkJggg=="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.classification_systems = []
        self.value_list = None
    
    def ensure_value_list(self):
        if self.Params.Input[0].Sources.Count == 0:
            archicad = tapir.Plugin.Archicad
            if archicad:
                self.classification_systems = archicad.GetAllClassificationSystems()
                items = [classification_system.name for classification_system in self.classification_systems]
                tapir.Factory.create_value_list(self, 0, "ClassificationSystem", items, 250)
                self.ExpireSolution(True)

    def RunScript(self, classificationSystems):
        self.ensure_value_list()
        for classification_system in self.classification_systems:
            if classification_system.name == classificationSystems:
                return classification_system

class GetElementsByClassification_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "GetElementsByClassification", "GetElementsByClassification", """Gets Element by Classification Systems""", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("58b0588c-fb46-4dc6-9ef0-32c6ea3cb90d")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_String()
        self.SetUpParam(p, "classificationSystems", "classificationSystems", "Script variable Python")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.item
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Names", "Names", "Script output Names.")
        self.Params.Output.Add(p)
        
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Elements", "Elements", "Script output Elements.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
                self.marshal.SetOutput(result[0], DA, 0, True)
                self.marshal.SetOutput(result[1], DA, 1, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAKtSURBVHgB3VU/bBJRGP/e3TUlsTQXczUNMhxLOUKVGztWJjfRaOpggbg5oZPaGtENYxpLbB0NHbthjDLWbkpNWmxJj6GBpQYrTS5h8Ejond/jTz2441J10f7CheN7v/d9v+/PewD87yB2RkmaTBICd+m7YcCiouw87eeIosi7XCMJICSOLBGJFXSX0TQuXalsqV0ea3UejEkBafH9u7eumZkbrvzGxrRhsOrh4cEns/Ph4TNrhCE38Sfflkp4lDvNDumX3SOeVVWtapYAgcBkAr8eLCw858fGBBCE9pPL5SRBOKfWat8LlDc+7r3fcW5XknGO07Va7WC9JwAtixTwp5JPHvOyHIKr166D0RJGIBqd5ZVSKULpdKMgjGVaigeBGCKKSdNX5tiGNV96mYbRUTc8nJuHcDgM2ewbcLvdMDf/CKKzt8Do9AXJIjji1zpjNu8qJYjFbkM+/xnq9TpmJUEq9Qz2979i4FGzgwo4B6hYAtBpWV5+BSsrr9Gxv6WePh7PeaCZLeEaQU6HnHH0b1rvGVNJuhgDosfRqNIlwzDoZNCPquvwoVTaadVVlmW+0WiuYY9ki2+ArcYP7lJ3VHumqFb7VhCEs9h9MoXOJTS5cMdHTRu6t7f3Zb3Lq1armtfrWW02da3TD75VFsywoXF3zOfActCoOk07iphtirKdgX8VrJ0xELjwAg/WVPew/A0sJcLRFIFwZWyWis3ymev5J2CsJjbZiYyXWTMBJ4TfH9ycmAiGHAO01dPbsQ16ckVR5uEE4DiIMwxk+4Mwduq7+J0sisVigWUh0h+E9Krnyv0b7XqBV3rZ+T4iFWV320ffuEHqbbI4/tNRlKLPjhsMBkNHOpNlGS5i2j9YvVMWTs6Lxc1C197qASHcFXBAOwvdkaPrJNPv/HTgJ1uWDACYoLhMAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.classification_systems = []
    
    def ensure_value_list(self):
        if self.Params.Input[0].Sources.Count == 0:
            archicad = tapir.Plugin.Archicad
            if archicad:
                self.classification_systems = archicad.GetAllClassificationSystems()
                items = [classification_system.name for classification_system in self.classification_systems]
                tapir.Factory.create_value_list(self, 0, "ClassificationSystem", items, 250)
                self.ExpireSolution(True)
    
    def RunScript(self, classificationSystems):
        try:
            self.ensure_value_list()
            for classification_system in self.classification_systems:
                if classification_system.name == classificationSystems:
                    #ClassificationSystemGuid = classification_system.guid
                    ClassificationsInSystem = tapir.Plugin.Archicad.GetAllClassificationsInSystem(classification_system)
                    names, elements = [], []
                    for classification_item in ClassificationsInSystem:
                        names.append([classification_item.id])
                        elements.append(tapir.Plugin.Archicad.GetElementsByClassification(classification_item.guid))
                    return list_to_tree(names), list_to_tree(elements)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), str(ex))

class GetBoundingBox_Component(component):
    
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "GetBoundingBox", "BBox", """Retrieves BoundingBox for the provided elements.""", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("0197343d-00a0-4956-a67f-418b3ca88d6b")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Elements", "E", "Elements as List.")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.list
        self.Params.Input.Add(p)
        
    def RegisterOutputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "bBox", "B", "BoundingBoxes as List.")
        self.Params.Output.Add(p)
        
    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAJcSURBVHgBzZVNTBNBFMff1F5LPHCxNXG9zcaSeHCb6MVKi3iC9uiB7uJNuglXqRrqwY8rKdWDH7CLZ6CeNGJSj8DBFEl2ORBi0jYxIaFpTfRgHedtS6ltaXdVCL9kksnMm///7ex7GYCTBKUXdnA4OUPsBFE6cBWAJXl00FpgkCWkqhiG8QX+BUqpwDOekwKXmabpbJ/FxSU2GLrOcE8UxXPgFEEQTlPqn5YCV/ZSqTQrl8uslXw+z3CPin6GsbaNRNE/ycfenamEJdILjMHY+v+R4fDr4AE8m7GYwlZX15hT0AjP1r7ooBBcDQdSmxJCwOfzgVMKxaJ1tlnrTwPG5uPx2xAZHYGYPA5TiXtQKBR7C/MYjFXVSQgNXgPUQK12gzrRaAR07RX4vF4IDw3DbPppR6NKpWLtYTIY+2HlHcRiY21xrk5Z4RWp6gSsvH/LxQsgK+OwtJxp7Gv6AoTCw9YeJoOxHo+nkxS4oQto9PjRQ0sIryHBByJJl2A2NQOBgAS9cIMN0EjX5qysEZzbxZbBPtHoKDjFBUfMcRowZbmpUv6WmgZT2g1492G9y8otWw3Wytr6+sHZpk5u/GTT+HzemjCXHB66kYxERgQ1PsEryNtVGAWx4TKZN6VfVZbc2tqcad4/1Xpgd/drrr/fmzFNo6QvvA7iGpZpX5/HyhLB+sdOfv7iJdxN3C/lchtPfnx339ze3vjYqtf1RaP0ogBQnfadPaPE+ddgwyHYten0M6iUv80T4k4axqdDXzabTyY3Ij+xu4L1pSx/Nh+Y5mYW/idUHNjBASeJ36MgYcqj2cC/AAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    @tapir.connect
    def RunScript(self, Elements):
        bBox = []
        if Elements:
            bBox = tapir.Plugin.Archicad.Get3DBoundingBoxes(Elements)
            success, message, bBox = tapir.RhinoWrapper.BoundingBox(bBox)
            
            if not success:
                self.AddRuntimeMessage(Grasshopper.Kernel.GH_RuntimeMessageLevel.Warning, message)
        return bBox

class HighlightElements_Component(component):
    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls, "HighlightElements", "Highlight", """Highlights elements on Archicad window.""", "tAPIr", "02 Get")
        return instance
    
    def get_ComponentGuid(self):
        return System.Guid("95091882-2129-4f1c-acdf-da7a325cc2bf")
    
    def SetUpParam(self, p, name, nickname, description):
        p.Name = name
        p.NickName = nickname
        p.Description = description
        p.Optional = True
    
    def RegisterInputParams(self, pManager):
        p = Grasshopper.Kernel.Parameters.Param_GenericObject()
        self.SetUpParam(p, "Elements", "E", "Elements to highlight")
        p.Access = Grasshopper.Kernel.GH_ParamAccess.list
        self.Params.Input.Add(p)
    
    def RegisterOutputParams(self, pManager):
        pass

    def SolveInstance(self, DA):
        p0 = self.marshal.GetInput(DA, 0)
        result = self.RunScript(p0)

        if result is not None:
            self.marshal.SetOutput(result, DA, 0, True)
        
    def get_Internal_Icon_24x24(self):
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsTAAALEwEAmpwYAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAIBSURBVHgBxVXPLztBFH8zre9tm2+jxNGJ3WyFgzZxUiTOji7ackf8BXohjqTcyx+gjSsSjtoD4kfrpAeVUBLZ5UCy+8xsrSq1O62IT7K78+PN570389l5AL8MTyPGqqr2+v3tHff35RvRNV5oAIZBxii1mseiaxpy4PFgBuAf/BiKEkx3dam9ovZ867q7g+l6c3UzoBQTJtA4a87ZY7LcMwiAkUqP7BUKJ/v2nGnSuNfrTdTjIuCCCrGZCoVDnYoigyRJkM3mIJfNFVko8Y+OGgYnD4UH8OAgi5+xvbOLfK4SQNMO1MsdRvQduGNuA82ARzYRnUQ3TETjjllQu8GVIMvB+ap6MBIO9wsEIoN9+F85Pjiw0dJS1bmu6+AGn08CJw6HyGq3KLW+gVdXJdQ0DTfTmffxaIxvUTACzUBWei5tBS0sLuHwyKilnGRy7V1J3AaahS3TfD5vESZX16yH4/w8/yZT5+ip0yT/iZ4eH8eisama8yiVrmF6ZhZ0TR8qFE73nDhcr+ty+fZC8vn72gKt8vPLizXGnWXSWylGvuK2noIACOKRVpNBib2xKLRWxIjru3JN8wyq37OzQ9e6IFQPDAP6GGkCETKcnBDC+kYKBAqPUAYcb2oZJIQ+IJrHbodrQ6gmswstxj7LLIMiU+h/lsF4INABd3c3f5/Br+MVbi1GePxGuTMAAAAASUVORK5CYII="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))
    
    @tapir.connect
    def RunScript(self, elements):        
        archicad = tapir.Plugin.Archicad
        if archicad:
            selected = []
            unselected = [237, 237, 237, 255]
            archicad.HighlightElements(elements)#, nonHighlightedColor=unselected)
