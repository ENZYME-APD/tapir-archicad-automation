using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateWorksheetMarkersComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateWorksheetMarkers";

        public CreateWorksheetMarkersComponent()
            : base(
                "CreateWorksheetMarkers",
                "Place Worksheet markers in the current floor plan, section or elevation, each with a new Worksheet viewpoint, like the Worksheet tool.",
                GroupNames.Navigator)
        {
        }

        protected override string ArrayKey => "worksheetMarkersData";

        protected override bool HasAdditionalSettingsInput => false;

        protected override bool SupportsElementMetadata => false;

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("ClipPolygons", "clipPolygon", FieldKind.OutlineCurve, "The closed boundary curve of the cut out area of each marker. Line and arc segments are kept as they are; anything else is approximated with a polyline. Only X and Y are used.", required: true),
            new Field("MarkerPoints", "markerCoordinate", FieldKind.Point2D, "Where the marker symbol is placed (only X and Y are used). Default is the first point of the clip polygon."),
            new Field("Angles", "angle", FieldKind.Number, "Rotation of the marker symbol in radians."),
            new Field("Names", "name", FieldKind.Text, "Name of the new viewpoint."),
            new Field("IDs", "id", FieldKind.Text, "Reference ID shown in the marker and in the Project Map; a '/' splits it between the two lines of the marker."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Story of the marker when it is placed on a floor plan.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // No icon of its own yet: the one of CreateWorksheets, which makes independent Worksheets.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWorksheets;

        public override Guid ComponentGuid =>
            new Guid("a8e4d2b7-1c5f-4e9a-8b3d-6f2c0e7a9d51");
    }
}
