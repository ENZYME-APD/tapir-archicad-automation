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
    public class InterpretAutoTextsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "InterpretAutoTexts";

        public InterpretAutoTextsComponent()
            : base(
                "InterpretAutoTexts",
                "Resolves autotexts (e.g. <LAYOUTID>) to their values, optionally in the context of an element, " +
                "so a key can be tested before it is written into a Text or Label.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "AutoTexts",
                "The autotexts to resolve, as they are written in a Text or Label (e.g. <LAYOUTID> or " +
                "<PROPERTY-69A58F6F-DD3B-478D-B5EF-09A16BD0C548>) or as bare keys (e.g. LAYOUTID).");

            InGeneric(
                "ElementGuid",
                "Identifier of the element in whose context the autotexts are resolved, e.g. a Text on a layout " +
                "for layout autotexts. Without it they are resolved without an element context. Optional.");

            SetOptionality(new[] { 1 });
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Values",
                "The resolved value of each autotext (empty when it could not be resolved).");

            OutErrorMessages(
                "Error message of each autotext (empty when it was resolved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> autoTexts) ||
                autoTexts.Count == 0)
            {
                return;
            }

            var parameters = new JObject { ["autoTexts"] = new JArray(autoTexts) };

            if (da.TryGet(
                    1,
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

            var values = new List<string>();
            var errors = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "values"))
            {
                values.Add(JsonOutputHelp.IsError(item) ? null : item["value"]?.ToString());
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, values);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAutoTextName;

        public override Guid ComponentGuid =>
            new Guid("37683f13-2ca6-4bb4-b0db-ac837b884a9e");
    }
}
