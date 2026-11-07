using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibraryPartCustomSectionComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraryPartCustomSection";

        public SetLibraryPartCustomSectionComponent()
            : base(
                "SetLibraryPartCustomSection",
                "Creates, replaces or deletes a custom section of a loaded library part. The content is given as base64. " +
                "A library part that is not editable is refused and left unchanged. Available from Archicad 28.",
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

            InInteger(
                "DataVersion",
                "Data version stored with the section; only used with Data. Optional, by default 0.");

            InText(
                "Data",
                "New content of the section, base64 encoded. The section is created when it does not exist, " +
                "otherwise replaced. Give either Data or Delete.");

            InBoolean(
                "Delete",
                "Delete the section. Give either Data or Delete.",
                false);

            SetOptionality(new[] { 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ErrorMessage",
                "Error message of the command (empty when it succeeded).");
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

            string data = null;
            var hasData = da.GetData(4, ref data) && data != null;
            var delete = da.GetOptional(5, false);
            if (hasData == delete)
            {
                this.AddError("Give either Data or Delete.");
                return;
            }

            if (hasData)
            {
                parameters["data"] = data;
                var dataVersion = 0;
                if (da.GetData(3, ref dataVersion))
                {
                    parameters["dataVersion"] = dataVersion;
                }
            }
            else
            {
                parameters["delete"] = true;
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
                LibraryComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraryPartCustomSection;

        public override Guid ComponentGuid =>
            new Guid("83adba00-70f6-49f6-bf89-ab805a9523d0");
    }
}
