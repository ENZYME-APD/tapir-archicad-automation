using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetAutoTextKeysComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAutoTextKeys";

        public GetAutoTextKeysComponent()
            : base(
                "GetAutoTextKeys",
                "Get the autotext keys a Text or Label can embed. Without an element only the keys " +
                "common to every element type are returned (Element ID, Area, ...); with an element " +
                "its own type's keys come on top (e.g. the thickness of a wall).",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ElementGuid",
                "Identifier of the element to get the context dependent keys for. Optional.");

            SetOptionality(new[] { 0 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each autotext, as shown in the Insert Autotext dialog of Archicad.");

            OutTexts(
                "Keys",
                "Key of each autotext, e.g. PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548.");

            OutTexts(
                "EmbeddableKeys",
                "Each key surrounded by < and >, ready to be put into the content of a Text or Label.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    0,
                    out GH_ObjectWrapper elementWrapper) &&
                elementWrapper != null)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrapper);
                if (elementId == null)
                {
                    this.AddError("Invalid ElementGuid.");
                    return;
                }
                parameters["elementId"] = new JObject { ["guid"] = elementId.Guid };
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var keys = new List<string>();
            var embeddableKeys = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "autoTextKeys"))
            {
                var key = item["key"]?.ToString() ?? "";
                names.Add(item["name"]?.ToString() ?? "");
                keys.Add(key);
                embeddableKeys.Add("<" + key + ">");
            }

            da.SetDataList(0, names);
            da.SetDataList(1, keys);
            da.SetDataList(2, embeddableKeys);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAutoTextKeys;

        public override Guid ComponentGuid =>
            new Guid("2ec34336-f275-4bd8-9a63-af648e61e466");
    }
}
