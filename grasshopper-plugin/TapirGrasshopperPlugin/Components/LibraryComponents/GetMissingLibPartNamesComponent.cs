using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetMissingLibPartNamesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMissingLibPartNames";

        public GetMissingLibPartNamesComponent()
            : base(
                "GetMissingLibPartNames",
                "Gets the library part name of the given placed Object, Lamp, Window or Door elements, even if the " +
                "library containing the part is not loaded. Use it to find the parts of elements that are shown as " +
                "dots because their library part is missing.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of placed Object, Lamp, Window or Door elements.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LibPartNames",
                "The name of the library part of each element, also when the library containing it is not loaded " +
                "(empty for an element that failed).");

            OutErrorMessages(
                "Error message of each element (empty when its library part name was read).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<GH_ObjectWrapper> wrappers) ||
                !this.TryGetElementIds(
                    wrappers,
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

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elements"] = elements },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var names = new List<string>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(response, "libPartNames"))
            {
                names.Add(JsonOutputHelp.IsError(item) ? null : item["libPartName"]?.ToString());
                errors.Add(JsonOutputHelp.ErrorMessage(item));
            }

            da.SetDataList(0, names);
            da.SetDataList(1, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("76d3463b-b85a-44ba-9c0f-98b05fad3879");
    }
}
