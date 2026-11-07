using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Components.NavigatorComponents;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibraryPartSectionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraryPartSection";

        public GetLibraryPartSectionComponent()
            : base(
                "GetLibraryPartSection",
                "Reads the text of a text section (a script or the comment) of a library part given by index, " +
                "without opening it in the GDL editor. Takes the password of a protected library part.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InInteger(
                "Index",
                "Index of the loaded library part, as GetAvailableLibraryParts returns it.");

            InText(
                "SectionType",
                "The four-character code of the section type, as GetLibraryPartSections returns it (e.g. '3D  '). " +
                "A shorter code is padded with spaces.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "SubIdent",
                "The sub-identifier of the section, as GetLibraryPartSections returns it. 0 by default. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "Password",
                "The password of a protected library part. Optional.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Content",
                "The text of the section.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var index = 0;
            string sectionType = null;
            if (!da.GetData(0, ref index) ||
                !da.GetData(1, ref sectionType) ||
                string.IsNullOrEmpty(sectionType))
            {
                return;
            }

            var parameters = new JObject
            {
                ["index"] = index,
                ["sectionType"] = sectionType
            };

            NavigatorComponentHelp.CopyOptional<int>(da, 2, parameters, "subIdent");
            NavigatorComponentHelp.CopyOptional<string>(da, 3, parameters, "password");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, response["content"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraryPartCustomSection;

        public override Guid ComponentGuid =>
            new Guid("c4e2f011-5a83-4791-9ba7-29dbfac052f2");
    }
}
