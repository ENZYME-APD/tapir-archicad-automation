using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyMeshesComponent : ModifyElementsComponentBase
    {
        public override string CommandName => "ModifyMeshes";

        public ModifyMeshesComponent()
            : base(
                "ModifyMeshes",
                "Modify Mesh elements. Only the connected optional inputs are changed on the elements. " +
                "The boundary polygon, holes and sublines can be modified through the AdditionalSettings " +
                "input (polygonCoordinates, polygonArcs, holes, sublines).",
                GroupNames.ElementModification,
                "meshesData",
                new List<Field>
                {
                    new Field("Levels", "level", FieldKind.Number, "Z reference level of the mesh coordinates."),
                    new Field("SkirtTypes", "skirtType", FieldKind.Text, "Skirt type: SurfaceOnlyWithoutSkirt, WithSkirt or SolidBodyWithSkirt."),
                    new Field("SkirtLevels", "skirtLevel", FieldKind.Number, "Height of the mesh skirt."),
                    new Field("Ridges", "ridges", FieldKind.Text, "Ridge type: AllSharp, AllSmooth or UserDefined."),
                    new Field("ShowLines", "showLines", FieldKind.Boolean, "Show the secondary mesh lines on the floor plan."),
                    new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the mesh."),
                    new Field("ContourPens", "contourPen", FieldKind.Integer, "Pen index of the mesh contour."),
                    new Field("LevelPens", "levelPen", FieldKind.Integer, "Pen index of the mesh level lines.")
                },
                itemWrapKey: "meshData")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMeshes;

        public override Guid ComponentGuid =>
            new Guid("f1808d03-c95c-4b54-b96a-cfc1b30a1562");
    }
}
