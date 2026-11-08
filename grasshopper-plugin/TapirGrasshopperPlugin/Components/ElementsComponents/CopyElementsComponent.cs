using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CopyElementsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CopyElements";

        public CopyElementsComponent()
            : base(
                "CopyElements",
                "Copy elements by an offset, like dragging a copy in Archicad, keeping all their settings. " +
                "With Count, copy n is moved by n times the offset, which makes a linear array. An element " +
                "Archicad cannot copy (a part of a curtain wall, stair or railing, a locked element or one on a " +
                "hidden or locked layer, ...) gets an error naming the reason, and the other elements are still " +
                "copied. All copies are one undo step.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to copy.");

            inManager.AddVectorParameter(
                "Offset",
                "Offset",
                "The vector in meters the first copy is moved by from its element.",
                GH_ParamAccess.item);

            InInteger(
                "Count",
                "Number of copies of each element (1 to 1000). Copy n is moved by n times the offset, so more " +
                "than one makes a linear array. Optional, default is 1.",
                1);

            SetOptionality(2);
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

            var offset = Vector3d.Zero;
            if (!da.GetData(1, ref offset))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elements"] = JObject.FromObject(elements)["elements"],
                ["offset"] = new JObject { ["x"] = offset.X, ["y"] = offset.Y, ["z"] = offset.Z }
            };

            // Only sent when it differs from the command's default.
            var count = da.GetOptional(
                2,
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

            SetCopiesOutputs(
                da,
                response);
        }

        // Hands out the "copies" of CopyElements and RotateCopyElements: one
        // branch of new elements and one error message per given element.
        internal static void SetCopiesOutputs(
            IGH_DataAccess da,
            JObject response)
        {
            var copies = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "copies");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                copies.EnsurePath(path);

                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        copies.Add(
                            ElementsStructuredGetterComponent.ElementIdOf(element),
                            path);
                    }
                }

                errors.Add(
                    JsonOutputHelp.IsError(item)
                        ? JsonOutputHelp.ErrorMessage(item)
                        : "");
            }

            da.SetDataTree(
                0,
                copies);
            da.SetDataList(
                1,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CopyElements;

        public override Guid ComponentGuid =>
            new Guid("bd252f11-bbf0-48a4-8265-ead5d38c017b");
    }
}
