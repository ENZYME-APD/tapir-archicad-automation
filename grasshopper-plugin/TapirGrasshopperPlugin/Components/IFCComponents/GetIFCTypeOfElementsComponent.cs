using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCTypeOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetIFCTypeOfElements";

        public GetIFCTypeOfElementsComponent()
            : base(
                "GetIFCTypeOfElements",
                "Retrieve the IFC types of the given elements.",
                GroupNames.IFC)
        {
        }

        protected override string ResponseArrayKey => "elementIFCTypes";

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried element.");

            OutTexts(
                "IFCTypes",
                "IFC type of each element.");

            OutTexts(
                "TypeObjectIFCTypes",
                "IFC type of the type object of each element.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var elementGuids = new List<object>();
            var ifcTypes = new List<object>();
            var typeObjectIfcTypes = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    ifcTypes.Add(null);
                    typeObjectIfcTypes.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementIdOf(item));
                ifcTypes.Add(JsonOutputHelp.Scalar(item, "ifcType"));
                typeObjectIfcTypes.Add(JsonOutputHelp.Scalar(item, "typeObjectIFCType"));
            }

            da.SetDataList(0, elementGuids);
            da.SetDataList(1, ifcTypes);
            da.SetDataList(2, typeObjectIfcTypes);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCTypeOfElements;

        public override Guid ComponentGuid =>
            new Guid("94fef72c-c0f0-40ac-9347-3c45ff81801f");
    }
}
