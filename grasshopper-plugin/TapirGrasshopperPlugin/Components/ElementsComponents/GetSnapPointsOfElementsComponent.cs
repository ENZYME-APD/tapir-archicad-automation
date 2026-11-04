using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSnapPointsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSnapPointsOfElements";

        public GetSnapPointsOfElementsComponent()
            : base(
                "SnapPointsOfElements",
                "Gets the snap points (hotspots) of elements of any type: the points the cursor snaps to and dimensions attach to.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the snap points of.");
        }

        protected override void AddOutputs()
        {
            OutPointTree(
                "SnapPoints",
                "The snap points of each element in world coordinates, one branch per element " +
                "(an empty branch for an element with an error).");

            OutIntegerTree(
                "NeigIds",
                "Which kind of point of the element each snap point is (the Archicad API_NeigID value), " +
                "one branch per element, aligned with SnapPoints.");

            OutIntegerTree(
                "InIndices",
                "Subindex of each snap point within its kind, for example the vertex index in the element's polygon, " +
                "one branch per element, aligned with SnapPoints.");

            OutErrorMessages(
                "Error message of each element (empty when its snap points were read).");
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

            if (!TryGetConvertedCadValues(
                    CommandName,
                    elements,
                    ToAddOn,
                    JHelp.Deserialize<SnapPointsOfElementsObject>,
                    out SnapPointsOfElementsObject response))
            {
                return;
            }

            var points = new DataTree<Point3d>();
            var neigIds = new DataTree<int>();
            var inIndices = new DataTree<int>();
            var errorMessages = new List<string>();

            for (var i = 0; i < response.SnapPointsOfElements.Count; i++)
            {
                var item = response.SnapPointsOfElements[i];
                var path = new GH_Path(i);
                points.EnsurePath(path);
                neigIds.EnsurePath(path);
                inIndices.EnsurePath(path);

                if (item?.Error != null)
                {
                    errorMessages.Add(item.Error.Message ?? "Unknown error.");
                    continue;
                }

                errorMessages.Add("");
                if (item?.SnapPoints == null)
                {
                    continue;
                }

                foreach (var hotspot in item.SnapPoints)
                {
                    if (hotspot?.Coordinate == null)
                    {
                        continue;
                    }

                    points.Add(
                        new Point3d(
                            hotspot.Coordinate.X,
                            hotspot.Coordinate.Y,
                            hotspot.Coordinate.Z),
                        path);
                    neigIds.Add(hotspot.NeigId, path);
                    inIndices.Add(hotspot.InIndex, path);
                }
            }

            da.SetDataTree(0, points);
            da.SetDataTree(1, neigIds);
            da.SetDataTree(2, inIndices);
            da.SetDataList(3, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateHotspots;

        public override Guid ComponentGuid =>
            new Guid("3f6b2a9e-5c41-4d8a-b7e2-9a1c4f0d6e83");
    }
}
