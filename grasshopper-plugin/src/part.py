#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

__all__ = ['GetElements_Component',
           'GetAllClassificationSystems_Component',
           'GetClassificationSystems_Component',
           'GetElementsByClassification_Component',
           'GetBoundingBox_Component']

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
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "GetElements", "GetElements", """Retrieves Element information from connected ArchiCAD project.""", "tAPIr", "02 Get")
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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAFiUAABYlAUlSJPAAAARLSURBVEhLrVQPTJRlGP/gO+5Q5O+x1ZBWrhVujSINnEVmSg2DcKgTOFNCIRtKTJuEUXFJhLRIxmHE0PnniggLExo5ScM/6CAIHFdkiYhwnod4B9wdf07w1/e83Ad3OoUxn+2353mf93me3/M+7/t9nL3I5XI8DNjK3Su0mZSUhJaWFuTl5TG7vb19Qouw94sQ11MSREdHs+CmpiaoVCpmiz7SIu63npIgNDSUBUdERKCmpobZok+0xTXphoYGh/WUBAEBASyYdGBg4IQtahEajQYZGRnMT5rW5J+SwMfHhyWRtrdJi6B1WFgY6z41NZVpWpP/XgI+ONKZDzFyXBDbfBgI5DgsdOGMC3guknPig417is7g8rURNP8zhNN/DqL6ggVHas04XGNGcbUZ+ZVm5B61IPMHC9JLx5GmNmP7ARPe3FoPb29vjFwpw2CjEv3HY9F76Dn8Eu+ChRLOIBwhCGebLFiSeBWvp3Qhasd1rP1Ej/U5N5G4x4AtRX2sEBX9+MgQlBXDDGSTLzK5jhH0lXrAcHAOeovd0KOahfaPpOwkjKDq9ACWvtOJ8NRurPxAhxilHvG5vdhcYERqyQDrlgpmVY5g96+3Gcgm34qkWkbQXy6H8bA7bpUIBIWz8G+6bJJAXdWHV20E0Tt1iNvVg3VZWigyL2Od8hI27GpDwmd/YVN2KxI/v4h4ZSPW7jiPqORaLI7KxxP+cqvp58dhVAsE+9xwc+9s/P2+3QkKvrvlQLB6ZyeWKvbf8Z/rZ6XuHoR5j/laj+1+8o6pws+BoDXFjiCzqMdhROFbNJjr52c9VZ6J0Z5ajOoqMapVY6yrEGOd2RhrT8No22ZYm2MwXLcMluqn0F/m6TCi5nftCLbm6BwuednGRtadtaMMph89Yan0xtBxHwz/JsfIKV8GsslHexRz9yX/keAySRCX1oXQhKsIS76GiO1avKw4zwiG24qEzjxgqvBihQarfVhRAtmsuLBHMWw8+4XxFM2GPt8VFxR2BMsTO/Diho6Je1i0avxlDDZno+9b93ESoUvzUS+Yj9kg2OSjPYoxHJjs/sZXrji3RjJO4OQcbPR//hDmL/8dz7x2Es+G1yBoxQlGYD63jSVSdzQCKjZQ7gmD2gP6EndoVXPQkeuG/5TCq0lzxcUUGRo3SVn3pa/w9KEJfwjhV+Fk+1XYgwgGTsSzrujoNF+6ROpqOqDi7FdxPyEC409vsCPTXBmR8EKM3z8N08m3MNSqwm19PUgo1pY2faGk3oMhbJ50aUREz4/eeHGCDIrFPL55W4rW96QzJ7hRMA/Xc6TQfSGD7ksZIyPELeKxagGPuBAeZ1dLZk7QleWO7k9d0J3lAm22lJERvo6VIPYFZ+yNkaDqJeeZETzi62UsXy9F54c8rqRLcGmbBJpkHs0bedQreNSt4XFmpTMKl8jwqBBrS5u++Pp6RVIidfcgUAzF2tLuEo77Hw8FEIyao6BZAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.state = 0
    
    @tapir.connect
    def RunScript(self, refresh):
        if self.state == 0:
            return tapir.Plugin.Archicad.GetAllElements()
        elif self.state == 1:
            return tapir.Plugin.Archicad.GetSelectedElements()

    def on_get_all_click(self, sender, args):
        try:
            # TODO: Remove Update input Parameter here.
            self.state = 0
            self.ExpireSolution(True)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)
    
    def on_get_selected_click(self, sender, args):
        try:
            # TODO: Add Update Input Parameter here.
            self.state = 1
            self.ExpireSolution(True)
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

    def AppendAdditionalMenuItems(self , items):
        component.AppendAdditionalMenuItems(self , items)
        try:
            item_a = items.Items.Add('Get All', None, self.on_get_all_click)
            item_b = items.Items.Add('Get Selected', None, self.on_get_selected_click)
            
            if self.state == 0:
                item_a.Checked = True
                item_b.Checked = False
            else:
                item_a.Checked = False
                item_b.Checked = True
            
        except Exception as ex:
            System.Windows.Forms.MessageBox.Show(traceback.format_exc(), ex)

class GetAllClassificationSystems_Component(component):

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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAAFiUAABYlAUlSJPAAAARLSURBVEhLrVQPTJRlGP/gO+5Q5O+x1ZBWrhVujSINnEVmSg2DcKgTOFNCIRtKTJuEUXFJhLRIxmHE0PnniggLExo5ScM/6CAIHFdkiYhwnod4B9wdf07w1/e83Ad3OoUxn+2353mf93me3/M+7/t9nL3I5XI8DNjK3Su0mZSUhJaWFuTl5TG7vb19Qouw94sQ11MSREdHs+CmpiaoVCpmiz7SIu63npIgNDSUBUdERKCmpobZok+0xTXphoYGh/WUBAEBASyYdGBg4IQtahEajQYZGRnMT5rW5J+SwMfHhyWRtrdJi6B1WFgY6z41NZVpWpP/XgI+ONKZDzFyXBDbfBgI5DgsdOGMC3guknPig417is7g8rURNP8zhNN/DqL6ggVHas04XGNGcbUZ+ZVm5B61IPMHC9JLx5GmNmP7ARPe3FoPb29vjFwpw2CjEv3HY9F76Dn8Eu+ChRLOIBwhCGebLFiSeBWvp3Qhasd1rP1Ej/U5N5G4x4AtRX2sEBX9+MgQlBXDDGSTLzK5jhH0lXrAcHAOeovd0KOahfaPpOwkjKDq9ACWvtOJ8NRurPxAhxilHvG5vdhcYERqyQDrlgpmVY5g96+3Gcgm34qkWkbQXy6H8bA7bpUIBIWz8G+6bJJAXdWHV20E0Tt1iNvVg3VZWigyL2Od8hI27GpDwmd/YVN2KxI/v4h4ZSPW7jiPqORaLI7KxxP+cqvp58dhVAsE+9xwc+9s/P2+3QkKvrvlQLB6ZyeWKvbf8Z/rZ6XuHoR5j/laj+1+8o6pws+BoDXFjiCzqMdhROFbNJjr52c9VZ6J0Z5ajOoqMapVY6yrEGOd2RhrT8No22ZYm2MwXLcMluqn0F/m6TCi5nftCLbm6BwuednGRtadtaMMph89Yan0xtBxHwz/JsfIKV8GsslHexRz9yX/keAySRCX1oXQhKsIS76GiO1avKw4zwiG24qEzjxgqvBihQarfVhRAtmsuLBHMWw8+4XxFM2GPt8VFxR2BMsTO/Diho6Je1i0avxlDDZno+9b93ESoUvzUS+Yj9kg2OSjPYoxHJjs/sZXrji3RjJO4OQcbPR//hDmL/8dz7x2Es+G1yBoxQlGYD63jSVSdzQCKjZQ7gmD2gP6EndoVXPQkeuG/5TCq0lzxcUUGRo3SVn3pa/w9KEJfwjhV+Fk+1XYgwgGTsSzrujoNF+6ROpqOqDi7FdxPyEC409vsCPTXBmR8EKM3z8N08m3MNSqwm19PUgo1pY2faGk3oMhbJ50aUREz4/eeHGCDIrFPL55W4rW96QzJ7hRMA/Xc6TQfSGD7ksZIyPELeKxagGPuBAeZ1dLZk7QleWO7k9d0J3lAm22lJERvo6VIPYFZ+yNkaDqJeeZETzi62UsXy9F54c8rqRLcGmbBJpkHs0bedQreNSt4XFmpTMKl8jwqBBrS5u++Pp6RVIidfcgUAzF2tLuEo77Hw8FEIyao6BZAAAAAElFTkSuQmCC"
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    @tapir.connect
    def RunScript(self, refresh):
        return tapir.Plugin.Archicad.GetAllClassificationSystems()

class GetClassificationSystems_Component(component):

    def __new__(cls):
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "GetClassificationSystems", "GetClassificationSystems", """Gets ClassificationSystems from current ArchiCAD project.""", "tAPIr", "99 WIP")
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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAASDSURBVEhLnZQLUJRVFMcXvmUXRN47I4OQOk5NM2YSBGalmWYDueKEjizrg0DK5GXaSNhUoERINcLwEHeIQClCVEhwyEcSomBMJINoRKiIgMhrP2B3eazgv+/c3YJ1khjOzH/m7P3uOb9zzz17RU8ye5lM7ujkzDs4OGAqOcqcedprDJu+OTrN4ZNV53C9eQTV9UM496sWJ8s1yD0zgNTjPBJy+hCd1g15yCkGMYZN36i6kkoNlvhmwO05BTzlh7H+o068vc8g8v323seanc3sJMaw6RsFqYr78dRiBdwW+WOBuxIBcV0IPNDNRP6mz7rgG9E0c8D+b9V4yf8IFnoosVyhQlBSL4K/6mMif2tiD9ZG3Zw5IPxQL7Yk9LCEoclq7EjlsTPdIPJpbV3U9f8BcF5yc85buCR3YdOErKzmmfz+Rx9kDzLtyhpEeGa/ALiG+ZaWWCwSmcjTQsR7cCK5yIzz4pMzK3Hr3ijq/hzGpWtDKLuqw4kKLY5d0EJVpkVKiRZJxTrEHtchJt+g6Dwt9uRosC6ihp1g9E4BhmrjMHBWgd6jS3AmyAKeYpFaOII7Lv+uw4rQu3gzso1NBl0e9ZdaQFVSIkr66YlhxBWNMJFPa/KwKgboz7eFOnc2elXW6E6zwu1PJOwkDFB6aRAr32uFz652Nn40IXSJ1GdqBVVLCeNLRnHwp4dM5NOa77sVDDBQ6AT+mA36sgRAuhX+ipFOAPJK+/G6EUAzTmO4Ob4Dythb2BzXhG0HGhH8+U1sT2hA6Bf1CIqrxaa91fALq8AyvxTMd3XSa36cBz5PAHxjjZ6MWfjjw0knSP2+zwSwYV8rViqzH7nOddFTdVNpgZtMf/rgwkeaIhcTQEPkJEBsZrdJi3zCb2Cui4u+vDAWY90VGOsswVhHHsbb0jHemoDx29EYa9wBfV0ARqpWQVf2NAYK7ExaVPf+JEBEYqfJJa8KqWXV6VsKoDlpB12JA4bPOmLkZyeMlsuYyKc1+kZ7Hr/k34ItJgCB0W14Nfgu3gi7h7V7OrBcWc0AI42ZQmW20BTZs0RDZY4sKYl8llz4RntYe7KF9mTOQleKJa4qJwFWh7bg5W0t/97DUn/DZAzVJaD/OxsDRKhSW2wP7WmjBJ/W6BvtUedMVP/gkCWubBQbAGbmXrzrC0fx7OpfsGjNRTzvcwHuvucZQHtlNwuk6qgFlGyw0A7qPFt0ZdmgI202WpKs0RwnTE20JeojpajdLmHV57/G0R9NeCGEp8LsP54KAgyeD2JV0dGpv3SJVNV0RMnZU/EkIwB/6i12ZOorAwkTwv/wDDQXt2C4IQ0Pu2pARnuNYdM3CurN9Wb9pEsjEI0fzbgqWArlMg5H3pGgIUoyc8CD1AW4nyhB55dSdH4tZTBS4FIO/h4cAr05XN4gnjmgLd4G7fst0B5vgY4ECYORDivEULxojowAMUpfMZ8ZYI7Mni/cKkHrxxzuxIjRtFuMG2Ec6kI41Cg5VG3kULneHOkrpHAW9hrDpm8ymb2cAqm6qUR7aK8x7DETif4GWQPKTm78ggMAAAAASUVORK5CYII="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.classification_systems = []
    
    def ensure_value_list(self):
        if self.Params.Input[0].Sources.Count == 0:
            self.classification_systems = tapir.Plugin.Archicad.GetAllClassificationSystems()
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
        instance = Grasshopper.Kernel.GH_Component.__new__(cls,
            "GetElementsByClassification", "GetElementsByClassification", """Gets Element by Classification Systems""", "tAPIr", "99 WIP")
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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAASDSURBVEhLnZQLUJRVFMcXvmUXRN47I4OQOk5NM2YSBGalmWYDueKEjizrg0DK5GXaSNhUoERINcLwEHeIQClCVEhwyEcSomBMJINoRKiIgMhrP2B3eazgv+/c3YJ1khjOzH/m7P3uOb9zzz17RU8ye5lM7ujkzDs4OGAqOcqcedprDJu+OTrN4ZNV53C9eQTV9UM496sWJ8s1yD0zgNTjPBJy+hCd1g15yCkGMYZN36i6kkoNlvhmwO05BTzlh7H+o068vc8g8v323seanc3sJMaw6RsFqYr78dRiBdwW+WOBuxIBcV0IPNDNRP6mz7rgG9E0c8D+b9V4yf8IFnoosVyhQlBSL4K/6mMif2tiD9ZG3Zw5IPxQL7Yk9LCEoclq7EjlsTPdIPJpbV3U9f8BcF5yc85buCR3YdOErKzmmfz+Rx9kDzLtyhpEeGa/ALiG+ZaWWCwSmcjTQsR7cCK5yIzz4pMzK3Hr3ijq/hzGpWtDKLuqw4kKLY5d0EJVpkVKiRZJxTrEHtchJt+g6Dwt9uRosC6ihp1g9E4BhmrjMHBWgd6jS3AmyAKeYpFaOII7Lv+uw4rQu3gzso1NBl0e9ZdaQFVSIkr66YlhxBWNMJFPa/KwKgboz7eFOnc2elXW6E6zwu1PJOwkDFB6aRAr32uFz652Nn40IXSJ1GdqBVVLCeNLRnHwp4dM5NOa77sVDDBQ6AT+mA36sgRAuhX+ipFOAPJK+/G6EUAzTmO4Ob4Dythb2BzXhG0HGhH8+U1sT2hA6Bf1CIqrxaa91fALq8AyvxTMd3XSa36cBz5PAHxjjZ6MWfjjw0knSP2+zwSwYV8rViqzH7nOddFTdVNpgZtMf/rgwkeaIhcTQEPkJEBsZrdJi3zCb2Cui4u+vDAWY90VGOsswVhHHsbb0jHemoDx29EYa9wBfV0ARqpWQVf2NAYK7ExaVPf+JEBEYqfJJa8KqWXV6VsKoDlpB12JA4bPOmLkZyeMlsuYyKc1+kZ7Hr/k34ItJgCB0W14Nfgu3gi7h7V7OrBcWc0AI42ZQmW20BTZs0RDZY4sKYl8llz4RntYe7KF9mTOQleKJa4qJwFWh7bg5W0t/97DUn/DZAzVJaD/OxsDRKhSW2wP7WmjBJ/W6BvtUedMVP/gkCWubBQbAGbmXrzrC0fx7OpfsGjNRTzvcwHuvucZQHtlNwuk6qgFlGyw0A7qPFt0ZdmgI202WpKs0RwnTE20JeojpajdLmHV57/G0R9NeCGEp8LsP54KAgyeD2JV0dGpv3SJVNV0RMnZU/EkIwB/6i12ZOorAwkTwv/wDDQXt2C4IQ0Pu2pARnuNYdM3CurN9Wb9pEsjEI0fzbgqWArlMg5H3pGgIUoyc8CD1AW4nyhB55dSdH4tZTBS4FIO/h4cAr05XN4gnjmgLd4G7fst0B5vgY4ECYORDivEULxojowAMUpfMZ8ZYI7Mni/cKkHrxxzuxIjRtFuMG2Ec6kI41Cg5VG3kULneHOkrpHAW9hrDpm8ymb2cAqm6qUR7aK8x7DETif4GWQPKTm78ggMAAAAASUVORK5CYII="
        return System.Drawing.Bitmap(System.IO.MemoryStream(System.Convert.FromBase64String(o)))

    def __init__(self):
        self.classification_systems = []
    
    def ensure_value_list(self):
        if self.Params.Input[0].Sources.Count == 0:
            self.classification_systems = tapir.Plugin.Archicad.GetAllClassificationSystems()
            items = [classification_system.name for classification_system in self.classification_systems]
            tapir.Factory.create_value_list(self, 0, "ClassificationSystem", items, 250)
            self.ExpireSolution(True)

    def RunScript(self, classificationSystems):
        self.ensure_value_list()
        for classification_system in self.classification_systems:
            if classification_system.name == classificationSystems:
                ClassificationSystemGuid = classification_system.guid
                ClassificationsInSystem = tapir.Plugin.Archicad.GetAllClassificationsInSystem(ClassificationSystemGuid)
                names, elements = [], []
                for classification_item in ClassificationsInSystem:
                    names.append([classification_item.id])
                    elements.append(tapir.Plugin.Archicad.GetElementsByClassification(classification_item.guid))
                
                return list_to_tree(names), list_to_tree(elements)

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
        o = "iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAABGdBTUEAALGPC/xhBQAAASlJREFUSEuVlAGSwyAMA5P/PzoFWQQZ25DuzQVjS4J0Or2Um+tbYdH6QNC4xl1mzD4rLF6tu6AfLNsUNaMOJjby4aQa5XmbIEVls64/OsDhhyMOyqSP1sFWDNRVWgXVLPrKvrHUBFPmrNIOp4TswH09bfC0qv1T0nrtMfov2GNGzGto33M7A5ax9gcPbn/Use5DHzoP8/CmEL+StkegBQGG9gqHDsabZl/r3mnBMrB6udmydngY3wo54w1X7OaPfixW95sNE6/Q+y7IhVpNacRuhNIolcZ75oJrbDP8UHeo58Pj+lxzrW/MnZo2bOcns6DSP2x7EDQfCaN/OHKO9TvOCsshwKhESdCXvC8ag7fW7IO5GGuCF83aW7ELeq8pyH5PInoACKbr+gEavy6K5E85VAAAAABJRU5ErkJggg=="
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