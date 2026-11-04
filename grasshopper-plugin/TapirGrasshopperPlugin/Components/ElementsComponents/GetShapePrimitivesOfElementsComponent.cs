using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetShapePrimitivesOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetShapePrimitivesOfElements";

        public GetShapePrimitivesOfElementsComponent()
            : base(
                "GetShapePrimitivesOfElements",
                "Returns the 2D drawing primitives of each given floor plan element, as GetDrawingPrimitivesOfElements " +
                "does, but drawn the way Archicad draws it by default (on the current story, clipped) instead of " +
                "on every story and unclipped. IncludeMarkers draws the objects attached to the elements (window " +
                "and door markers, drawing titles, ...) with them or leaves them out. The floor plan must be the " +
                "current database. The output is aligned with the input list of elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of floor plan elements.");

            // No default: an unconnected input keeps Archicad's own setting.
            inManager.AddBooleanParameter(
                "IncludeMarkers",
                "IncludeMarkers",
                "Whether the objects attached to the elements (window and door markers, drawing titles, ...) " +
                "are drawn with them. Optional: Archicad's current setting when unconnected.",
                GH_ParamAccess.item);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            DrawingPrimitivesComponentHelp.AddOutputs(
                this,
                " (one branch per element)");

            OutErrorMessages(
                "Error message of each element (empty when successful).");
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

            var parameters = JObject.FromObject(elements);
            var includeMarkers = false;
            if (da.GetData(
                    1,
                    ref includeMarkers))
            {
                parameters["includeMarkers"] = includeMarkers;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = JsonOutputHelp.Items(
                response,
                "shapePrimitivesOfElements");
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

        // Shares the icon of GetDrawingPrimitivesOfElements, whose output it has.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDrawingPrimitivesOfElements;

        public override Guid ComponentGuid =>
            new Guid("3b1fbc5c-9bab-4c90-89ce-8c27ce152ba5");
    }
}
