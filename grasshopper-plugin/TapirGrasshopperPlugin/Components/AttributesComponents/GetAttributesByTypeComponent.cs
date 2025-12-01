using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

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
            InText("AttributeType");
        }

        protected override void AddOutputs()
        {
            OutGenerics("AttributeGuids");
            OutGenerics("AttributeIndices");
            OutGenerics("AttributeNames");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new AttributeTypeValueList().AddAsSource(
                this,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string aType))
            {
                return;
            }

            if (!TryGetConvertedValues(
                    CommandName,
                    new AttributesByTypeObj { AttributeType = aType },
                    ToAddOn,
                    JHelp.Deserialize<AttributeDetailsObj>,
                    out AttributeDetailsObj response))
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