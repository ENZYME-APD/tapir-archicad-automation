using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetAvailableLibraryPartsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAvailableLibraryParts";

        public GetAvailableLibraryPartsComponent()
            : base(
                "GetAvailableLibraryParts",
                "List library parts currently available to the project. Optionally filter by type (e.g. Door, Window, Object, Lamp).",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "FilterByTypeId",
                "Optional. Filter by library part type (e.g. Door, Window, Object, Lamp).");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutText(
                "LibraryParts",
                "JSON object with the available library parts.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (da.TryGet(
                    0,
                    out string filter) &&
                !string.IsNullOrEmpty(filter))
            {
                parameters["filterByTypeId"] = filter;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response.ToString(Formatting.Indented));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAvailableLibraryParts;

        public override Guid ComponentGuid =>
            new Guid("3f1ff684-73e0-445f-962b-791efd70a776");
    }
}
