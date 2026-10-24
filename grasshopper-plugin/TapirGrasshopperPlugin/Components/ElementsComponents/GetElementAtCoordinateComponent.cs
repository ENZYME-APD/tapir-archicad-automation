using Grasshopper.Kernel;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementAtCoordinateComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementAtCoordinate";

        public GetElementAtCoordinateComponent()
            : base(
                "ElementAtCoordinate",
                "Finds the element of the given polygonal type (for example Slab, Roof, Mesh, Zone or Hatch) at a point of the floor plan, the way a click there would.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ElementType",
                "The type of the element to look for: a polygonal type, for example Slab, Roof, Mesh, Zone or Hatch.");

            InPoint(
                "Point",
                "The point to search at, in Archicad's project coordinates. Its Z is handed to Archicad as the elevation of the point.");

            InTexts(
                "Filters",
                defaultValue: nameof(ElementFilter.NoFilter),
                description: "Only an element passing every given filter is found.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "ElementGuid",
                "The identifier of the found element, or null when nothing was found.");

            OutBoolean(
                "Found",
                "True when an element of the type was found at the point.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string elementType))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out Point3d point))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> filters))
            {
                return;
            }

            var input = new ElementAtCoordinateInput
            {
                ElementType = elementType,
                Coordinate = Point2D.Create(point),
                Z = point.Z,
                Filters = filters
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    JHelp.Deserialize<ElementAtCoordinateOutput>,
                    out ElementAtCoordinateOutput response))
            {
                return;
            }

            if (response.Found && response.ElementId != null)
            {
                da.SetData(
                    0,
                    new ElementGuidWrapper
                    {
                        ElementId = response.ElementId
                    });
            }

            da.SetData(
                1,
                response.Found);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ElemsByType;

        public override Guid ComponentGuid =>
            new Guid("3b7e5c1a-9d42-4f0b-8a6e-2c1f7d9e4b53");
    }
}
