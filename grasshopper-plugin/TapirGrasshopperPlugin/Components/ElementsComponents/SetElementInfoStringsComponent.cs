using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetElementInfoStringsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetElementInfoStrings";

        public SetElementInfoStringsComponent()
            : base(
                "SetElementInfoStrings",
                "Sets the info string of the given elements: the Element ID shown in the Info Box, which " +
                "GetDetailsOfElements returns as id.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to set the Element ID of.");

            InTexts(
                "ElementIDs",
                "The new Element ID of each element, as the Info Box shows it (input only 1 to use the same " +
                "ID for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when its Element ID was set).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> infoStrings) ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    infoStrings,
                    elementIds.Count,
                    "ElementIDs"))
            {
                return;
            }

            var elements = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                elements.Add(
                    new JObject
                    {
                        ["elementId"] = elementIds[i],
                        ["infoString"] = infoStrings.At(i) ?? ""
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elements"] = elements },
                ToAddOn,
                da);
        }

        // Shares the icon of ElementDetails until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemDetails;

        public override Guid ComponentGuid =>
            new Guid("ddad45d0-509e-406a-9465-de4e30b3d3bc");
    }
}
