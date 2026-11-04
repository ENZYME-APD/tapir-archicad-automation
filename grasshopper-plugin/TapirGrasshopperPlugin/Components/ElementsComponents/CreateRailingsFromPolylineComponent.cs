using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.ToolSettings;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // Creates railings along a 3D reference polyline. The Railings tool
    // component (Design Tools) creates them from a plan polyline with the
    // settings of the Railing Settings dialog; this one gives each node of the
    // reference line its own elevation. The inputs of the Railing settings are
    // named after the labels of the Railing Settings dialog
    // (ToolSettings/Railing.json).
    public class CreateRailingsFromPolylineComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateRailingsFromPolyline";

        public CreateRailingsFromPolylineComponent()
            : base(
                "CreateRailingsFromPolyline",
                "Create Railing elements along a 3D reference polyline: each node has its own elevation " +
                "(its Z, relative to the floor level of the home story), so the railing can climb or descend " +
                "along its segments. Posts, rails, handrails, toprails, panels and balusters come from the " +
                "favorite or from the Railing tool defaults. Arc segments can be given through the " +
                "AdditionalSettings input (polylineArcs; begIndex and endIndex must be consecutive).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "railingsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Polylines", "polylineCoordinates", FieldKind.PointsTree3D, "The nodes of the reference line of each railing (one branch per railing, at least 2 points). Z is the elevation of the node relative to the floor level of the home story.", required: true, minPointsPerBranch: 2),
            new Field(ToolSettingsTable.LabelOr("Railing", "height", "Height"), "height", FieldKind.Number, "Height of the railing segments. When not given, the segment height of the favorite (or of the Railing tool defaults) is kept."),
            new Field(ToolSettingsTable.LabelOr("Railing", "floorIndex", "FloorIndex"), "floorIndex", FieldKind.Integer, "Home story index of the railing. When not given, the current story."),
            new Field("ElementIDs", "id", FieldKind.Text, "Element ID of the new railing (the ID field of the Settings dialog). When not given, the element gets the next automatic ID of its tool."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // Shares the icon of CreateRailings until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateRailings;

        public override Guid ComponentGuid =>
            new Guid("c55a85c2-23d3-4314-b8e6-f62deb53234c");
    }
}
