using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetAttributesByTypeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAttributesByType";

        public GetAttributesByTypeComponent()
            : base(
                "AttributesByType",
                "Get all attributes by Type",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText("Type");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "Guids",
                "List of attribute Guids.");
            OutGenerics(
                "Indices",
                "List of attribute indices.");
            OutGenerics(
                "Names",
                "List of attribute names.");
            outManager.AddTimeParameter(
                "ModificationTimes",
                "ModificationTimes",
                "Last modification time of each attribute, in local time.",
                GH_ParamAccess.list);
            // Archicad 30 reports how, from where, by whom and when an attribute was created, but only for
            // an attribute it keeps a creation record of; the outputs below are null for the others and on
            // older versions, so the lists stay aligned with the Guids.
            OutTexts(
                "CreationMethods",
                "Creation method of each attribute (AddOn, Merge, Paste, New, Import, Explode, Duplicate, Hotlink, XRef, Library, LibraryMaster or Unspecified). Archicad 30 or later; null when unknown.");
            OutTexts(
                "CreationMethodDescriptions",
                "Description of the creation method of each attribute. Archicad 30 or later; null when unknown.");
            OutTexts(
                "CreationSources",
                "Origin of each attribute, e.g. the file or Add-On it came from. Archicad 30 or later; null when unknown.");
            OutTexts(
                "CreatorUserNames",
                "Teamwork user name, or OS login name in a solo project, that created each attribute. Archicad 30 or later; null when unknown.");
            outManager.AddTimeParameter(
                "CreationTimes",
                "CreationTimes",
                "Creation time of each attribute, in local time. Archicad 30 or later; null when unknown.",
                GH_ParamAccess.list);
            OutTexts(
                "FolderPaths",
                "Attribute folder of each attribute, the folder names joined with '/'; empty for the root folder (and always before Archicad 26).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<AttributeTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string attributeType))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { attributeType },
                    ToAddOn,
                    JHelp.Deserialize<AttributeDetailsObject>,
                    out AttributeDetailsObject response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Attributes.Select(x => x.AttributeId));

            da.SetDataList(
                1,
                response.Attributes.Select(x => x.Index));

            da.SetDataList(
                2,
                response.Attributes.Select(x => x.Name));

            da.SetDataList(
                3,
                response.Attributes.Select(
                    x => DateTimeOffset.FromUnixTimeSeconds(x.ModificationTime).LocalDateTime));

            da.SetDataList(
                4,
                response.Attributes.Select(x => x.CreationInfo?.Method));

            da.SetDataList(
                5,
                response.Attributes.Select(x => x.CreationInfo?.MethodDescription));

            da.SetDataList(
                6,
                response.Attributes.Select(x => x.CreationInfo?.Source));

            da.SetDataList(
                7,
                response.Attributes.Select(x => x.CreationInfo?.CreatorUserName));

            da.SetDataList(
                8,
                response.Attributes.Select(
                    x => x.CreationInfo == null
                        ? (DateTime?)null
                        : DateTimeOffset.FromUnixTimeSeconds(x.CreationInfo.CreationTime).LocalDateTime));

            // Appended after the creation outputs, so saved definitions keep their wiring. An add-on older than this output sends no folderPath; such an attribute reads as root.
            da.SetDataList(
                9,
                response.Attributes.Select(
                    x => x.FolderPath == null ? string.Empty : string.Join("/", x.FolderPath)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttributesByType;

        public override Guid ComponentGuid =>
            new Guid("f974c9ec-e3ec-4cf2-a576-b7a8548c9883");
    }
}