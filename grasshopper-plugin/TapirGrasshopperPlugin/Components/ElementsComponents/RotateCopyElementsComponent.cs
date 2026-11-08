using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class RotateCopyElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RotateCopyElements";

        public RotateCopyElementsComponent()
            : base(
                "RotateCopyElements",
                "Copy elements rotated around a center on the floor plan, like rotating a copy in Archicad, " +
                "keeping all their settings. With Count, copy n is rotated by n times the angle, which makes a " +
                "circular array. An element Archicad cannot copy gets an error naming the reason, and the other " +
                "elements are still copied. All copies are one undo step.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to copy.");

            inManager.AddPointParameter(
                "Center",
                "Center",
                "The center of the rotation on the floor plan (only X and Y are used).",
                GH_ParamAccess.item);

            inManager.AddNumberParameter(
                "Angle",
                "Angle",
                "The angle in radians the first copy is rotated by from its element, counterclockwise.",
                GH_ParamAccess.item);

            InInteger(
                "Count",
                "Number of copies of each element (1 to 1000). Copy n is rotated by n times the angle, so more " +
                "than one makes a circular array. Optional, default is 1.",
                1);

            SetOptionality(3);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "The new elements made from each given element, the first copy first (one branch per input " +
                "element). When the element is in a group and Suspend Groups is off, Archicad copies the whole " +
                "group, and the copies of the other members are listed too.");

            OutErrorMessages(
                "Error message of each element (empty when every copy was made). The copies made before an " +
                "error are still listed in ElementGuids.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var center = Point3d.Unset;
            if (!da.GetData(1, ref center))
            {
                return;
            }

            var angle = 0.0;
            if (!da.GetData(2, ref angle))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = JObject.FromObject(elements)["elements"],
                ["center"] = new JObject { ["x"] = center.X, ["y"] = center.Y },
                ["angle"] = angle
            };

            // Only sent when it differs from the command's default.
            var count = da.GetOptional(
                3,
                1);
            if (count != 1)
            {
                parameters["count"] = count;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            CopyElementsComponent.SetCopiesOutputs(
                da,
                response);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RotateCopyElements;

        public override Guid ComponentGuid =>
            new Guid("b0c33b63-dc73-429b-861c-ef885b6168ab");
    }
}
