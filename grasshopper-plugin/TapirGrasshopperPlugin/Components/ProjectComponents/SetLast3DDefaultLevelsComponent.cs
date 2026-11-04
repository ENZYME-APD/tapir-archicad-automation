using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetLast3DDefaultLevelsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLast3DDefaultLevels";

        public SetLast3DDefaultLevelsComponent()
            : base(
                "SetLast3DDefaultLevels",
                "Sets the last 3D default levels of the given element types' defaults.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "The element types to set the last 3D default level of (e.g. Wall, Slab).");

            InNumbers(
                "Levels",
                "The new last 3D default level of each element type, in the order of ElementTypes, in meters. " +
                "A single value is used for every element type.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes) ||
                elementTypes.Count == 0)
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<double> levels) ||
                levels.Count == 0)
            {
                return;
            }

            if (levels.Count == 1 && elementTypes.Count > 1)
            {
                var level = levels[0];
                levels = new List<double>();
                foreach (var _ in elementTypes)
                {
                    levels.Add(level);
                }
            }

            if (levels.Count != elementTypes.Count)
            {
                this.AddError(
                    "Levels must hold one value, or one value per item of ElementTypes.");
                return;
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes),
                ["levels"] = JArray.FromObject(levels)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to set the last 3D default levels.");
            }
        }

        // Shares the icon of SetStories until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetStories;

        public override Guid ComponentGuid =>
            new Guid("0877ade4-4dae-402c-bc6d-562a1000e744");
    }
}
