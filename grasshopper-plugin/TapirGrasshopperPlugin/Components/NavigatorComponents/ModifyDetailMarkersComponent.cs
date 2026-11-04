using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ModifyDetailMarkersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyDetailMarkers";

        public ModifyDetailMarkersComponent()
            : base(
                "ModifyDetailMarkers",
                "Change the name, the reference ID and the clip polygon of Detail and Worksheet markers.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Detail or Worksheet markers to change.");

            InTexts(
                "Names",
                "New name of each viewpoint (input only 1 to use the same name for all). Optional, the name is kept without it.");

            InTexts(
                "IDs",
                "New reference ID of each marker, shown in the marker and in the Project Map; a '/' splits it between the two lines of the marker (input only 1 to use the same ID for all). Optional, the ID is kept without it.");

            inManager.AddCurveParameter(
                "ClipPolygons",
                "ClipPolygons",
                "New closed boundary curve of each marker (input only 1 to use the same boundary for all). Line and arc segments are kept as they are; anything else is approximated with a polyline. Only X and Y are used. Optional, the boundary is kept without it.",
                GH_ParamAccess.list);

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each marker (empty when it was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            da.TryGetList(1, out List<string> names);
            names = names ?? new List<string>();
            da.TryGetList(2, out List<string> ids);
            ids = ids ?? new List<string>();
            var curves = new List<Curve>();
            da.GetDataList(3, curves);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("Names", names.Count),
                         ("IDs", ids.Count),
                         ("ClipPolygons", curves.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != elementWrappers.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input ElementGuids.");
                    return;
                }
            }

            var polygons = new List<(JArray Coordinates, JArray Arcs)>();
            foreach (var curve in curves)
            {
                if (!CreateElementsComponentBase.TryConvertCurveToPolygon(curve, out JArray coordinates, out JArray arcs, out string error))
                {
                    this.AddError($"The input ClipPolygons is invalid: {error}.");
                    return;
                }
                polygons.Add((coordinates, arcs));
            }

            var items = new JArray();
            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError("Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var item = new JObject
                {
                    ["elementId"] = new JObject { ["guid"] = elementId.Guid }
                };

                if (names.Count > 0)
                {
                    item["name"] = names[names.Count == 1 ? 0 : i];
                }

                if (ids.Count > 0)
                {
                    item["id"] = ids[ids.Count == 1 ? 0 : i];
                }

                if (polygons.Count > 0)
                {
                    var polygon = polygons[polygons.Count == 1 ? 0 : i];
                    item["clipPolygon"] = polygon.Coordinates.DeepClone();
                    if (polygon.Arcs.Count > 0)
                    {
                        item["polygonArcs"] = polygon.Arcs.DeepClone();
                    }
                }

                items.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["detailMarkersWithDetails"] = items },
                ToAddOn,
                da);
        }

        // No icon of its own yet: the one of CreateDetails.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDetails;

        public override Guid ComponentGuid =>
            new Guid("c41e7a95-2b3d-4f86-a0c9-7d5e1b8f3a62");
    }
}
