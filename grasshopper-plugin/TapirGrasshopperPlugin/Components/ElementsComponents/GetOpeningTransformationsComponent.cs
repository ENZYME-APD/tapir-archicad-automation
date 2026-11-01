using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetOpeningTransformationsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetOpeningTransformations";

        public GetOpeningTransformationsComponent()
            : base(
                "GetOpeningTransformations",
                "Gets the placement of Opening elements, Windows, Doors and Skylights without changing them: a " +
                "global-to-local transformation matrix, its inverse, and the origin and axes in world coordinates " +
                "derived from the inverse. For Windows, Doors and Skylights the local-to-global matrix is the one " +
                "Archicad returns, and mirrored ones give a left-handed matrix (IsMirrored). For Opening elements " +
                "it is built from the opening geometry (Archicad 29 and later): origin at the anchor point, the X " +
                "and Y axes of the opening and Z = X x Y, which points against the extrusion direction. Needs " +
                "Archicad 27 or later.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "openingTransformations";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Opening, Window, Door and Skylight elements to read.");
        }

        protected override void AddOutputs()
        {
            OutNumbers(
                "GlobalToLocal",
                "The global-to-local transformation of each element: a 3x4 matrix of 12 numbers, row by row, the " +
                "4th, 8th and 12th being the translation (one branch per element).");

            OutNumbers(
                "LocalToGlobal",
                "The local-to-global transformation of each element, in the same format (one branch per element).");

            OutPoints(
                "Origins",
                "The origin of each element's local coordinate system in world coordinates.");

            OutVectors(
                "XAxes",
                "The local X axis of each element in world coordinates.");

            OutVectors(
                "YAxes",
                "The local Y axis of each element in world coordinates.");

            OutVectors(
                "ZAxes",
                "The local Z axis of each element in world coordinates.");

            OutNumberList(
                "Determinants",
                "The determinant of the 3x3 part of each local-to-global matrix: 1, or -1 for a mirrored element.");

            OutBooleans(
                "IsMirrored",
                "Whether each element is mirrored (its local coordinate system is left-handed).");

            OutErrorMessages(
                "Error message of each element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var globalToLocal = new DataTree<double>();
            var localToGlobal = new DataTree<double>();
            var origins = new List<object>();
            var xAxes = new List<Vector3d?>();
            var yAxes = new List<Vector3d?>();
            var zAxes = new List<Vector3d?>();
            var determinants = new List<double?>();
            var isMirrored = new List<bool?>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                globalToLocal.EnsurePath(path);
                localToGlobal.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    origins.Add(null);
                    xAxes.Add(null);
                    yAxes.Add(null);
                    zAxes.Add(null);
                    determinants.Add(null);
                    isMirrored.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                AddNumbers(globalToLocal, item["globalToLocal"], path);
                AddNumbers(localToGlobal, item["localToGlobal"], path);
                origins.Add(JsonOutputHelp.Point(item["origin"]));
                xAxes.Add(GetOpeningDetailsComponent.VectorOf(item["xAxis"]));
                yAxes.Add(GetOpeningDetailsComponent.VectorOf(item["yAxis"]));
                zAxes.Add(GetOpeningDetailsComponent.VectorOf(item["zAxis"]));
                determinants.Add(item.Value<double?>("determinant"));
                isMirrored.Add(item.Value<bool?>("isMirrored"));
                errors.Add("");
            }

            da.SetDataTree(0, globalToLocal);
            da.SetDataTree(1, localToGlobal);
            da.SetDataList(2, origins);
            da.SetDataList(3, xAxes);
            da.SetDataList(4, yAxes);
            da.SetDataList(5, zAxes);
            da.SetDataList(6, determinants);
            da.SetDataList(7, isMirrored);
            da.SetDataList(8, errors);
        }

        private static void AddNumbers(
            DataTree<double> tree,
            JToken numbers,
            GH_Path path)
        {
            if (!(numbers is JArray array))
            {
                return;
            }

            foreach (var number in array)
            {
                tree.Add((double) number, path);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetOpeningTransformations;

        public override Guid ComponentGuid =>
            new Guid("358bce69-f391-42e0-b415-b7477469af2a");
    }
}
