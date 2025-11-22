using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetAttributesByTypeComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get all attributes by type";
        public override string CommandName => "GetAttributesByType";

        public GetAttributesByTypeComponent()
            : base(
                "Attributes By Type",
                "AttributesByType",
                Doc,
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Type",
                "Attribute type.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "List of attribute Guids.");

            OutGenerics(
                "AttributeIndices",
                "List of attribute indices.");

            OutGenerics(
                "AttributeNames",
                "List of attribute names.");
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
            if (!da.GetItem(
                    0,
                    out string attrType))
            {
                return;
            }

            var attributesByType =
                new AttributesByTypeObj { AttributeType = attrType };

            if (!GetConvertedResponse(
                    CommandName,
                    attributesByType,
                    out AttributeDetailsObj attributes)) { return; }

            var attributeIds = new List<AttributeIdObj>();
            var attributeIndices = new List<uint>();
            var attributeNames = new List<string>();

            foreach (var attributeDetail in attributes.Attributes)
            {
                attributeIds.Add(attributeDetail.AttributeId);
                attributeIndices.Add(attributeDetail.Index);
                attributeNames.Add(attributeDetail.Name);
            }

            da.SetDataList(
                0,
                attributeIds);
            da.SetDataList(
                1,
                attributeIndices);
            da.SetDataList(
                2,
                attributeNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AttributesByType;

        public override Guid ComponentGuid =>
            new Guid("f974c9ec-e3ec-4cf2-a576-b7a8548c9883");
    }
}