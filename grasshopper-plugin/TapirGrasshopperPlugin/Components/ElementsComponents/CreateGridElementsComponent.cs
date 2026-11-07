using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateGridElementsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateGridElements";

        public CreateGridElementsComponent()
            : base(
                "CreateGridElements",
                "Create grid elements as the Grid Tool places them, from the Grid Tool default. " +
                "Each grid line runs from its start point to its end point, and its length goes to the " +
                "GDL parameter AC_Length of the grid marker. GDL parameters and the length mode (the parameters " +
                "and lengthMode fields) are given through the AdditionalSettings input, for example the " +
                "ParameterSettings output of GetGridElements to make a grid look like an existing one; parameters " +
                "that cannot be set are reported as warnings. Available from Archicad 26.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "gridElements";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("StartPoints", "begCoordinate", FieldKind.Point2D, "Start point of each grid line, where the grid marker is inserted (only X and Y are used).", required: true),
            new Field("EndPoints", "endCoordinate", FieldKind.Point2D, "End point of each grid line (only X and Y are used). It sets the direction and the length.", required: true),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the grid element."),
            new Field("LayerIndices", "layerIndex", FieldKind.Integer, "Layer attribute index to place the grid element on.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // The GDL parameters that were not set (unknown name, wrong value type)
        // come back as warnings, prefixed with the index of the grid element.
        protected override void OnResponse(
            JObject response)
        {
            if (response?["warnings"] is JArray warnings)
            {
                foreach (var warning in warnings)
                {
                    AddRuntimeMessage(
                        GH_RuntimeMessageLevel.Warning,
                        warning.ToString());
                }
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateGridElements;

        public override Guid ComponentGuid =>
            new Guid("cfd69bfb-8c9a-45c9-83b9-cd1b7bad3675");
    }
}
