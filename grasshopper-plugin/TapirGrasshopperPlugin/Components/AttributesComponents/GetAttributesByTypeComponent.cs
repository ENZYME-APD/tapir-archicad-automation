using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class AttributesByTypeObj
    {
        [JsonProperty ("attributeType")]
        public string AttributeType;
    }

    public class GetAttributesByTypeComponent : ArchicadAccessorComponent
    {
        public GetAttributesByTypeComponent ()
          : base (
                "Attributes By Type",
                "AttributesByType",
                "Get all attributes by type.",
                "Attributes"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Type", "Type", "Attribute type.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("AttributeGuids", "AttributeGuids", "List of attribute Guids.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("AttributeIndices", "AttributeIndices", "List of attribute indices.", GH_ParamAccess.list);
            pManager.AddGenericParameter ("AttributeNames", "AttributeNames", "List of attribute names.", GH_ParamAccess.list);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new AttributeTypeValueList ().AddAsSource (this, 0);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            string attrType = "";
            if (!DA.GetData (0, ref attrType)) {
                return;
            }

            AttributesByTypeObj attributesByType = new AttributesByTypeObj () {
                AttributeType = attrType
            };
            JObject attributesByTypeObj = JObject.FromObject (attributesByType);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetAttributesByType", attributesByTypeObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
            AttributesObj attributes = response.Result.ToObject<AttributesObj> ();
            List<AttributeIdObj> attributeIds = new List<AttributeIdObj> ();
            List<uint> attributeIndices = new List<uint> ();
            List<string> attributeNames = new List<string> ();
            foreach (AttributeDetail attributeDetail in attributes.Attributes) {
                attributeIds.Add (attributeDetail.AttributeId);
                attributeIndices.Add (attributeDetail.Index);
                attributeNames.Add (attributeDetail.Name);
            }
            DA.SetDataList (0, attributeIds);
            DA.SetDataList (1, attributeIndices);
            DA.SetDataList (2, attributeNames);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.AttributesByType;

        public override Guid ComponentGuid => new Guid ("f974c9ec-e3ec-4cf2-a576-b7a8548c9883");
    }
}