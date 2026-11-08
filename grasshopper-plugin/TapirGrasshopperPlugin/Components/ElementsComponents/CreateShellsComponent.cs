using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateShellsComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateShells";

        public CreateShellsComponent()
            : base(
                "CreateShells",
                "Create extruded, revolved and ruled Shell elements from profile polylines. The base plane, " +
                "the extrusion start and the plane tilts, the revolution axis and angles, the second profile " +
                "and the profile planes of a ruled shell, the clipping contour, the holes and the edge trims " +
                "can be given through the AdditionalSettings input, in the fields of the CreateShells command.",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "shellsData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("ProfilePoints", "profile", FieldKind.PointsTree2D, "Points of the profile polyline of each shell (one branch per shell, at least 2 points; only X and Y are used). Extruded: in the plane across the extrusion, X horizontal and Y up. Revolved: in the base plane's XY plane, X the radius and Y the height. Ruled: in the XY plane of profilePlane (AdditionalSettings).", required: true, minPointsPerBranch: 2),
            new Field("ShellClasses", "shellClass", FieldKind.Text, "Shell class: Extruded (the profile swept along ExtrusionVectors), Revolved (the profile turned around the axis) or Ruled (a surface ruled between the profile and secondProfile, given with profilePlane and secondProfilePlane through AdditionalSettings).", required: true),
            new Field("ProfileClosed", ProfileClosedKey, FieldKind.Boolean, "Whether the segment from the last point of the profile back to the first one is part of the profile. False by default."),
            new Field("ExtrusionVectors", "extrusionVector", FieldKind.Point3D, "Extruded only, required for it: the direction and length of the extrusion as a point (its X, Y and Z), in base plane coordinates."),
            new Field("RevolutionAngles", "revolutionAngle", FieldKind.Number, "Revolved only: the angle the profile is turned through, in radians. 2*pi (a full turn) by default."),
            new Field("Thicknesses", "thickness", FieldKind.Number, "Thickness of the shell. Only takes effect on a Basic shell; a Composite shell takes its thickness from the composite."),
            new Field("IsFlipped", "isFlipped", FieldKind.Boolean, "Applies the thickness to the other side of the reference surface."),
            new Field("StructureTypes", "structureType", FieldKind.Text, "Structure type: Basic or Composite.", valueList: () => new StructureTypeValueList ()),
            new Field("BuildingMaterialGuids", "buildingMaterialId", FieldKind.AttributeGuid, "Building material attribute for Basic structure."),
            new Field("CompositeGuids", "compositeId", FieldKind.AttributeGuid, "Composite attribute for Composite structure."),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the shell. Without it the shell is placed on the story of the Shell tool defaults.")
        };

        // ProfileClosed is written under this key first and moved into the
        // profile object by CompleteItem, as the profile is still the point
        // array of ProfilePoints when the typed inputs are written.
        private const string ProfileClosedKey = "profileClosed";

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        // The command takes the profile as a ShellProfile object
        // ({ "coordinates": [...], "closed": ... }), not as a bare point array.
        protected override string CompleteItem(
            JObject item)
        {
            if (!(item["profile"] is JArray coordinates))
            {
                return "Every shell needs the points of its profile in ProfilePoints.";
            }

            var profile = new JObject { ["coordinates"] = coordinates.DeepClone() };
            var closed = item[ProfileClosedKey];
            if (closed != null)
            {
                profile["closed"] = closed.DeepClone();
                item.Remove(ProfileClosedKey);
            }

            item["profile"] = profile;
            return null;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateShells;

        public override Guid ComponentGuid =>
            new Guid("a78a746c-b821-4035-a6d2-4e4b67f2e7d6");
    }
}
