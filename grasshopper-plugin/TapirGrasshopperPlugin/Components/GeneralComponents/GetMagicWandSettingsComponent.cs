using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetMagicWandSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMagicWandSettings";

        public GetMagicWandSettingsComponent()
            : base(
                "GetMagicWandSettings",
                "Returns the settings of Archicad's Magic Wand Settings dialog: how curves are approximated with " +
                "straight segments and the trace method. Changes nothing in the project.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "CurveApproximation",
                "How the Magic Wand approximates curves with straight segments: SegmentsAlongArcs or " +
                "SegmentsAlongCircles (SegmentCount segments), DeviationFromCurves (at most MaxDeviation from " +
                "the curve) or SegmentLength (segments of SegmentLength). Unknown for a mode Tapir does not know.");

            OutInteger(
                "SegmentCount",
                "The number of segments along an arc or circle.");

            outManager.AddNumberParameter(
                "MaxDeviation",
                "MaxDeviation",
                "The largest distance of the segments from the curve, in meters.".WithTypeName("numberItem"),
                GH_ParamAccess.item);

            outManager.AddNumberParameter(
                "SegmentLength",
                "SegmentLength",
                "The length of the segments, in meters.".WithTypeName("numberItem"),
                GH_ParamAccess.item);

            OutText(
                "TraceMethod",
                "How curves are traced: Linear (with straight segments as set above) or BestMatch (with the " +
                "curves that match them best). Unknown for a method Tapir does not know.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["curveApproximation"]?.ToString());

            da.SetData(
                1,
                (int?)response["segmentCount"]);

            da.SetData(
                2,
                (double?)response["maxDeviation"]);

            da.SetData(
                3,
                (double?)response["segmentLength"]);

            da.SetData(
                4,
                response["traceMethod"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMagicWandSettings;

        public override Guid ComponentGuid =>
            new Guid("506c5c2a-d272-4fe0-a52a-d5431f2255ab");
    }
}
