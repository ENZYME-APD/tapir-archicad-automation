using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetLibPartTransformationsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetLibPartTransformations";

        public GetLibPartTransformationsComponent()
            : base(
                "GetLibPartTransformations",
                "Gets the placement transformation of library part instances (Window, Door, Object, Lamp, " +
                "Zone) as Archicad computes it: the location, the rotation angle and the x/y scaling. The " +
                "output is aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "transformations";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of library part instances (Window, Door, Object, Lamp, Zone).");
        }

        protected override void AddOutputs()
        {
            OutPoints(
                "Locations",
                "The location of each element on the floor plan.");

            OutNumberList(
                "Angles",
                "The rotation angle of each element in radians.");

            OutNumberList(
                "XRatios",
                "The scaling of each element along its x axis.");

            OutNumberList(
                "YRatios",
                "The scaling of each element along its y axis.");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var locations = new List<object>();
            var angles = new List<object>();
            var xRatios = new List<object>();
            var yRatios = new List<object>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                var transformation = item?["transformation"];
                if (JsonOutputHelp.IsError(item) || transformation == null)
                {
                    locations.Add(null);
                    angles.Add(null);
                    xRatios.Add(null);
                    yRatios.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                locations.Add(JsonOutputHelp.Point(transformation["location"]));
                angles.Add(
                    JsonOutputHelp.Scalar(
                        transformation,
                        "angle"));
                xRatios.Add(
                    JsonOutputHelp.Scalar(
                        transformation,
                        "xRatio"));
                yRatios.Add(
                    JsonOutputHelp.Scalar(
                        transformation,
                        "yRatio"));
                errors.Add("");
            }

            da.SetDataList(
                0,
                locations);
            da.SetDataList(
                1,
                angles);
            da.SetDataList(
                2,
                xRatios);
            da.SetDataList(
                3,
                yRatios);
            da.SetDataList(
                4,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibPartTransformations;

        public override Guid ComponentGuid =>
            new Guid("eb4eadf8-0442-402e-8b67-e1e8b9bb7831");
    }
}
