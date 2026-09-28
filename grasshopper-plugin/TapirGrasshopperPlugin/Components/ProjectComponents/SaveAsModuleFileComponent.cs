using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SaveAsModuleFileComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SaveAsModuleFile";

        public SaveAsModuleFileComponent()
            : base(
                "SaveAsModuleFile",
                "Save elements into a .mod module file. Left empty, ElementGuids saves the " +
                "current selection, as Save Selection as Module would; Archicad 25 and 26 " +
                "support that form only. The current window has to be a floor plan, section, " +
                "elevation or detail.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ModuleFilePath",
                "Absolute path of the .mod file to write. An existing file is overwritten.");

            InGenerics(
                "ElementGuids",
                "Identifiers of the elements that go into the module. Optional.");

            SetOptionality(new[] { 1 });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string moduleFilePath = null;
            if (!da.GetData(0, ref moduleFilePath) ||
                string.IsNullOrWhiteSpace(moduleFilePath))
            {
                return;
            }

            var parameters = new JObject { ["moduleFilePath"] = moduleFilePath };

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> elementWrappers);

            if (elementWrappers.Count > 0)
            {
                var elements = new JArray();
                foreach (var wrapper in elementWrappers)
                {
                    var elementId = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
                    if (elementId == null)
                    {
                        this.AddError(
                            "Invalid element identifier in the ElementGuids input.");
                        return;
                    }
                    elements.Add(new JObject
                    {
                        ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                    });
                }
                parameters["elements"] = elements;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SaveAsModuleFile;

        public override Guid ComponentGuid =>
            new Guid("a906f091-1822-b738-85ec-fb9eb35bc844");
    }
}
