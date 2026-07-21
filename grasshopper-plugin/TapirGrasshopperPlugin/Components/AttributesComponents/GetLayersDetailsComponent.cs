using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetLayersDetailsComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetLayers";

        public GetLayersDetailsComponent()
            : base(
                "GetLayersDetails",
                "Get the details of the given Layer attributes.")
        {
        }

        protected override string ResponseArrayKey => "layers";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each layer attribute.");

            OutIntegers(
                "Indices",
                "Index of each layer attribute.");

            OutTexts(
                "Names",
                "Name of each layer attribute.");

            OutBooleans(
                "IsHidden",
                "True if the layer is hidden.");

            OutBooleans(
                "IsLocked",
                "True if the layer is locked.");

            OutBooleans(
                "IsWireframe",
                "True if the layer is shown in wireframe mode.");

            OutIntegers(
                "IntersectionGroupNrs",
                "Intersection group number of each layer.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var guids = new List<object>();
            var indices = new List<object>();
            var names = new List<object>();
            var isHidden = new List<object>();
            var isLocked = new List<object>();
            var isWireframe = new List<object>();
            var intersectionGroupNrs = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    isHidden.Add(null);
                    isLocked.Add(null);
                    isWireframe.Add(null);
                    intersectionGroupNrs.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                isHidden.Add(JsonOutputHelp.Scalar(item, "isHidden"));
                isLocked.Add(JsonOutputHelp.Scalar(item, "isLocked"));
                isWireframe.Add(JsonOutputHelp.Scalar(item, "isWireframe"));
                intersectionGroupNrs.Add(JsonOutputHelp.Scalar(item, "intersectionGroupNr"));
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, isHidden);
            da.SetDataList(4, isLocked);
            da.SetDataList(5, isWireframe);
            da.SetDataList(6, intersectionGroupNrs);
            da.SetDataList(7, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLayers;

        public override Guid ComponentGuid =>
            new Guid("fa0448c4-a557-4a87-8bb1-6efa6a948169");
    }
}
