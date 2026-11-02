using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The outputs GetDrawingPrimitivesOfElements and GetStoryControlPrimitives
    // share; the error messages of the former follow them.
    public static class DrawingPrimitivesComponentHelp
    {
        public static void AddOutputs(
            Component component,
            string branches)
        {
            component.OutTextTree(
                "Types",
                "The type of each primitive. " + DrawingPrimitivesOutput.OutputsDescription + branches);

            component.OutGenericTree(
                "Geometries",
                "The geometry of each primitive on the XY plane: a point for Point and Text (the location), " +
                "a line, an arc or ellipse, a polyline for PLine and Tri, a planar surface (outline with " +
                "holes) for Poly and the destination box of a Pict, without its rotation. Null for the " +
                "control items." + branches);

            component.OutIntegerTree(
                "Pens",
                "The pen of each primitive." + branches);

            component.OutGenericTree(
                "Layers",
                "The layer of each primitive." + branches);

            component.OutIntegerTree(
                "DrawIndices",
                "The drawing order index of each primitive." + branches);

            component.OutTextTree(
                "RenovationStatuses",
                "The renovation status of each primitive: Undefined, Existing, New or Demolished." +
                branches);

            component.OutTextTree(
                "Texts",
                "The content of each Text primitive (null for the others)." + branches);

            component.OutTextTree(
                "Primitives",
                "Every field of each primitive as JSON: the line type, determination and pen weight, the " +
                "fill of a Poly (fillPen, rgbColor), the font, size and anchor of a Text, the hatch of a " +
                "Ctrl_HatchBorderBeg or Ctrl_HatchLinesBeg item, etc." + branches);
        }

        public static void SetOutputs(
            IGH_DataAccess da,
            DrawingPrimitivesOutput output)
        {
            da.SetDataTree(
                0,
                output.Types);
            da.SetDataTree(
                1,
                output.Geometries);
            da.SetDataTree(
                2,
                output.Pens);
            da.SetDataTree(
                3,
                output.Layers);
            da.SetDataTree(
                4,
                output.DrawIndices);
            da.SetDataTree(
                5,
                output.RenovationStatuses);
            da.SetDataTree(
                6,
                output.Texts);
            da.SetDataTree(
                7,
                output.Primitives);
        }
    }

    public class GetDrawingPrimitivesOfElementsComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetDrawingPrimitivesOfElements";

        public GetDrawingPrimitivesOfElementsComponent()
            : base(
                "GetDrawingPrimitivesOfElements",
                "Returns the 2D drawing primitives (points, lines, arcs, texts, polylines, triangles, polygons, " +
                "pictures) Archicad draws each given floor plan element from, in drawing order, decomposed on " +
                "every story and unclipped. Control items (type Ctrl_*) mark the beginning and the end of " +
                "sub-parts such as hatches and texts. The floor plan must be the current database. The " +
                "output is aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "drawingPrimitivesOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of floor plan elements.");
        }

        protected override void AddOutputs()
        {
            DrawingPrimitivesComponentHelp.AddOutputs(
                this,
                " (one branch per element)");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var output = new DrawingPrimitivesOutput();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                if (JsonOutputHelp.IsError(item))
                {
                    output.EnsurePath(path);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                output.AddPrimitives(
                    item["primitives"],
                    path);
                errors.Add("");
            }

            DrawingPrimitivesComponentHelp.SetOutputs(
                da,
                output);
            da.SetDataList(
                8,
                errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDrawingPrimitivesOfElements;

        public override Guid ComponentGuid =>
            new Guid("e8ba42b5-525c-4721-9eb8-b4a29dec285e");
    }
}
