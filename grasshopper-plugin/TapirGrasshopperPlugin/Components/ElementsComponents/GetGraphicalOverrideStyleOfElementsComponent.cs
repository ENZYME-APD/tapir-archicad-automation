using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGraphicalOverrideStyleOfElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetGraphicalOverrideStyleOfElements";

        public GetGraphicalOverrideStyleOfElementsComponent()
            : base(
                "GraphicalOverrideStyleOfElements",
                "Gets the graphic override style each element is drawn with in the current window of Archicad: " +
                "for every overridable member (line type, pens, fill types, surfaces, ...) whether the active " +
                "graphic override combination replaces it, and with what. Available from Archicad 27.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query. The current window must show them.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "OverrideStyles",
                "Graphic override style of each element as JSON. Every member that can be overridden holds " +
                "'overridden' and, when it is, the attributeId, penIndex or color it is overridden with.");

            OutBooleans(
                "Overridden",
                "Whether the active graphic override combination overrides any member of the style of each element.");

            OutErrorMessages(
                "Error message of each element (empty when its style was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(input),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var styles = new List<string>();
            var overridden = new List<bool?>();
            var errors = new List<string>();

            var items = response?["styles"] as JArray ?? new JArray();
            foreach (var item in items)
            {
                if (item?["error"] != null)
                {
                    styles.Add(null);
                    overridden.Add(null);
                    errors.Add(
                        item["error"]?["message"]?.ToString() ??
                        "Unknown error.");
                    continue;
                }

                styles.Add(item?.ToString(Formatting.None));
                overridden.Add(
                    item is JObject style &&
                    style.Properties().Any(x =>
                        x.Value is JObject member &&
                        member["overridden"]?.Type == JTokenType.Boolean &&
                        member.Value<bool>("overridden")));
                errors.Add("");
            }

            da.SetDataList(
                0,
                styles);

            da.SetDataList(
                1,
                overridden);

            da.SetDataList(
                2,
                errors);
        }

        // Shares the icon of ElementDetails until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemDetails;

        public override Guid ComponentGuid =>
            new Guid("3a9d5f72-c6e8-4b10-8d47-e2f19a6b0c85");
    }
}
