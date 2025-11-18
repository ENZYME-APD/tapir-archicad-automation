using Eto.Forms;
using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class AttributesByTypeObj
    {
        [JsonProperty("attributeType")]
        public string AttributeType;
    }

    public class GetAttributesByTypeComponent : ArchicadAccessorComponent
    {
        public override string Doc => "Get all attributes by type";
        public override string CommandName => "GetAttributesByType";

        public GetAttributesByTypeComponent()
            : base(
                "Attributes By Type",
                "AttributesByType",
                GroupNames.Attributes)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Type",
                "Type",
                "Attribute type.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "AttributeGuids",
                "AttributeGuids",
                "List of attribute Guids.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "AttributeIndices",
                "AttributeIndices",
                "List of attribute indices.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "AttributeNames",
                "AttributeNames",
                "List of attribute names.",
                GH_ParamAccess.list);
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

            AttributesByTypeObj attributesByType =
                new() { AttributeType = attrType };

            if (!GetConvertedResponse(
                    CommandName,
                    attributesByType,
                    out AttributeDetailsObj attributes)) { return; }

            List<AttributeIdObj> attributeIds = new();
            List<uint> attributeIndices = new();
            List<string> attributeNames = new();
            foreach (AttributeDetail attributeDetail in attributes.Attributes)
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
            new("f974c9ec-e3ec-4cf2-a576-b7a8548c9883");
    }
}