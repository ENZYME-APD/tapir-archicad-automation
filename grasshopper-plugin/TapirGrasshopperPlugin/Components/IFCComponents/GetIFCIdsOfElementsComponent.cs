using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCIdsOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetIFCIdsOfElements";

        public GetIFCIdsOfElementsComponent()
            : base(
                "GetIFCIdsOfElements",
                "Retrieve the IFC identifiers of the given elements.",
                GroupNames.IFC)
        {
        }

        protected override string ResponseArrayKey => "elementIFCIds";

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried element.");

            OutTexts(
                "IFCIds",
                "IFC identifier of each element.");

            OutTexts(
                "ExternalIFCIds",
                "External IFC identifier of each element.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var elementGuids = new List<object>();
            var ifcIds = new List<object>();
            var externalIfcIds = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    ifcIds.Add(null);
                    externalIfcIds.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementIdOf(item));
                ifcIds.Add(JsonOutputHelp.Scalar(item, "ifcId"));
                externalIfcIds.Add(JsonOutputHelp.Scalar(item, "externalIFCId"));
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, ifcIds);
            da.SetDataList(2, externalIfcIds);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCIdsOfElements;

        public override Guid ComponentGuid =>
            new Guid("c81c04eb-8f87-417e-b1e6-8008f8ee5db6");
    }
}
