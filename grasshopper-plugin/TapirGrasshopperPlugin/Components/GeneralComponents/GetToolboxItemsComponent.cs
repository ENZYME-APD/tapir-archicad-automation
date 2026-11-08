using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetToolboxItemsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetToolboxItems";

        public GetToolboxItemsComponent()
            : base(
                "GetToolboxItems",
                "Lists the tools of the Toolbox, the tools of add-ons included, and the active tool. The " +
                "ElementTypes, Variations and ClassIds outputs are parallel lists, one item per tool, so the " +
                "items at one index can be connected to SetCurrentTool to activate that tool.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "IncludeHidden",
                "Also list the tools that have no icon in the Toolbox but can be active, e.g. the Slab " +
                "Accessory. Optional; default: false.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "ElementTypes",
                "The element type of each tool (e.g. Wall, Door, Object). Unknown for the tools of add-ons, " +
                "which ClassIds identifies.");

            OutTexts(
                "Variations",
                "The four-character subtype of each tool (e.g. SKYL, CRWD). Empty for the generic tool of " +
                "the element type.");

            OutTexts(
                "ClassIds",
                "The class id (a guid) of each tool of an add-on. Empty for the other tools. Archicad 26 and later.");

            OutText(
                "CurrentElementType",
                "The element type of the active tool. Empty for the Arrow and the Marquee.");

            OutText(
                "CurrentVariation",
                "The variation of the active tool. Empty for the generic tool of the element type.");

            OutText(
                "CurrentClassId",
                "The class id of the active tool when it is an add-on's tool.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            // Unconnected leaves it false, the command's default.
            da.TryGet(
                0,
                out bool includeHidden);

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["includeHidden"] = includeHidden },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementTypes = new List<string>();
            var variations = new List<string>();
            var classIds = new List<string>();

            if (response?["toolboxItems"] is JArray items)
            {
                foreach (var item in items)
                {
                    elementTypes.Add(item?["elementType"]?.ToString());
                    variations.Add(item?["variation"]?.ToString());
                    classIds.Add(item?["classId"]?.ToString());
                }
            }

            da.SetDataList(
                0,
                elementTypes);

            da.SetDataList(
                1,
                variations);

            da.SetDataList(
                2,
                classIds);

            var currentTool = response?["currentTool"];

            da.SetData(
                3,
                currentTool?["elementType"]?.ToString());

            da.SetData(
                4,
                currentTool?["variation"]?.ToString());

            da.SetData(
                5,
                currentTool?["classId"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertiesOfToolDefaults;

        public override Guid ComponentGuid =>
            new Guid("e3a96c47-51b8-4d2f-8e7c-0b4f6a9d1c82");
    }
}
