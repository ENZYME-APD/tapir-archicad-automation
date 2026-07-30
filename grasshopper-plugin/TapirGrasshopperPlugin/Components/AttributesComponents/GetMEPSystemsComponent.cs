using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetMEPSystemsComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetMEPSystems";

        public GetMEPSystemsComponent()
            : base(
                "GetMEPSystems",
                "Get the details of the given MEP System attributes.")
        {
        }

        protected override string ResponseArrayKey => "mepSystems";

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each MEP system attribute.");

            OutIntegers(
                "Indices",
                "Index of each MEP system attribute.");

            OutTexts(
                "Names",
                "Name of each MEP system attribute.");

            OutTextTree(
                "Domains",
                "Domains of each MEP system: Ventilation, Piping or CableCarrier (one branch per attribute).");

            OutIntegers(
                "ContourPens",
                "Contour pen of each MEP system.");

            OutIntegers(
                "FillPens",
                "Fill pen of each MEP system.");

            OutIntegers(
                "FillBackgroundPens",
                "Fill background pen of each MEP system.");

            OutIntegers(
                "CenterLinePens",
                "Center line pen of each MEP system.");

            OutGenerics(
                "FillGuids",
                "Identifier of the fill attribute of each MEP system.");

            OutGenerics(
                "CenterLineTypeGuids",
                "Identifier of the center line type attribute of each MEP system.");

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
            var domains = new DataTree<object>();
            var contourPens = new List<object>();
            var fillPens = new List<object>();
            var fillBackgroundPens = new List<object>();
            var centerLinePens = new List<object>();
            var fillGuids = new List<object>();
            var centerLineTypeGuids = new List<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                domains.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    contourPens.Add(null);
                    fillPens.Add(null);
                    fillBackgroundPens.Add(null);
                    centerLinePens.Add(null);
                    fillGuids.Add(null);
                    centerLineTypeGuids.Add(null);
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                contourPens.Add(JsonOutputHelp.Scalar(item, "contourPen"));
                fillPens.Add(JsonOutputHelp.Scalar(item, "fillPen"));
                fillBackgroundPens.Add(JsonOutputHelp.Scalar(item, "fillBackgroundPen"));
                centerLinePens.Add(JsonOutputHelp.Scalar(item, "centerLinePen"));
                fillGuids.Add(WrappedAttributeIdOf(item["fillId"]));
                centerLineTypeGuids.Add(WrappedAttributeIdOf(item["centerLineTypeId"]));

                if (item["domain"] is JArray domainArray)
                {
                    foreach (var domain in domainArray)
                    {
                        domains.Add(
                            domain.ToString(),
                            path);
                    }
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataTree(3, domains);
            da.SetDataList(4, contourPens);
            da.SetDataList(5, fillPens);
            da.SetDataList(6, fillBackgroundPens);
            da.SetDataList(7, centerLinePens);
            da.SetDataList(8, fillGuids);
            da.SetDataList(9, centerLineTypeGuids);
            da.SetDataList(10, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPSystems;

        public override Guid ComponentGuid =>
            new Guid("7b0e3bed-9ebd-45b3-8239-6f77686a56ca");
    }
}
