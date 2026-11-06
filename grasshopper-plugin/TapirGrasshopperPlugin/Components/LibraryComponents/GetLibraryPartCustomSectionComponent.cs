using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartCustomSectionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartCustomSection";

        public GetLibraryPartCustomSectionComponent()
            : base(
                "GetLibraryPartCustomSection",
                "Gets a custom section of a loaded library part by its section identifier and index: whether it exists, " +
                "its data version and its content as base64. Available from Archicad 28.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "Name of the loaded library part file with its extension, as the fileName of GetAvailableLibraryParts.");

            InGeneric(
                "SectionId",
                "Type identifier of the custom section: an integer, or a four-character code as text " +
                "(e.g. 'ABCD', which is 1094861636).");

            InInteger(
                "Index",
                "Index of the section among the sections with the same identifier.",
                0);

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Exists",
                "Whether the library part has this custom section.");

            OutInteger(
                "DataVersion",
                "The data version stored with the section (empty when the section does not exist).");

            OutText(
                "Data",
                "The content of the section, base64 encoded (empty when the section does not exist).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string name = null;
            if (!da.GetData(0, ref name) ||
                !LibraryComponentHelp.TryGetSectionId(this, da, 1, out var sectionId))
            {
                return;
            }

            var parameters = new JObject
            {
                ["name"] = name,
                ["sectionId"] = sectionId
            };

            var index = 0;
            if (da.GetData(2, ref index))
            {
                parameters["index"] = index;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, (bool?)response["exists"] ?? false);
            da.SetData(1, (int?)response["dataVersion"]);
            da.SetData(2, response["data"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartCustomSection;

        public override Guid ComponentGuid =>
            new Guid("6ef00b77-1260-4344-b42d-b6419be313df");
    }
}
