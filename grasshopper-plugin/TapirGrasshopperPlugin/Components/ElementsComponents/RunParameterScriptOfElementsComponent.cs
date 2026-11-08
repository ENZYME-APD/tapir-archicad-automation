using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RunParameterScriptOfElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RunParameterScriptOfElements";

        public RunParameterScriptOfElementsComponent()
            : base(
                "RunParameterScriptOfElements",
                "Runs the parameter script of the library part of the given elements (objects, lamps, windows, " +
                "doors, etc.), so that parameter values that depend on other parameters are updated, e.g. after " +
                "SetGDLParameters.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to run the parameter script of.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when the script ran successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            var elements = new JArray();
            foreach (var elementId in elementIds)
            {
                elements.Add(new JObject { ["elementId"] = elementId });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elements"] = elements },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RunParameterScriptOfElements;

        public override Guid ComponentGuid =>
            new Guid("2885187c-bc6a-4042-a7c6-1305cd4d6588");
    }
}
