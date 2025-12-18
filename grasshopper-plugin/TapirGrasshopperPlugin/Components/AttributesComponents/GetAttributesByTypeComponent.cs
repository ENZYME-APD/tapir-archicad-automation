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
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttributesByType;

        public override Guid ComponentGuid =>
            new Guid("f974c9ec-e3ec-4cf2-a576-b7a8548c9883");
    }
}