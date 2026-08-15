using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateMeshesComponent : CreateElementsComponentBase
    {
        public override string CommandName => "CreateMeshes";

        public CreateMeshesComponent()
            : base(
                "CreateMeshes",
                "Create Mesh elements based on the given parameters. " +
                "Holes, arcs and sublines can be given through the AdditionalSettings input " +
                "(polygonArcs, holes, sublines).",
                GroupNames.ElementCreation)
        {
        }

        protected override string ArrayKey => "meshesData";

        private static readonly List<Field> FieldDefinitions = new List<Field>
        {
            new Field("Vertices", "polygonCoordinates", FieldKind.PointsTree3D, "Boundary points of each mesh (one branch per mesh, at least 3 points).", required: true, minPointsPerBranch: 3),
            new Field("Levels", "level", FieldKind.Number, "Z reference level of the mesh coordinates."),
            new Field("SkirtTypes", "skirtType", FieldKind.Text, "Skirt type: SurfaceOnlyWithoutSkirt, WithSkirt or SolidBodyWithSkirt.", valueList: () => new MeshSkirtTypeValueList ()),
            new Field("SkirtLevels", "skirtLevel", FieldKind.Number, "Height of the mesh skirt."),
            new Field("Ridges", "ridges", FieldKind.Text, "Ridge type: AllSharp, AllSmooth or UserDefined.", valueList: () => new MeshRidgeTypeValueList ()),
            new Field("FloorIndices", "floorIndex", FieldKind.Integer, "Home story index of the mesh."),
            new Field("FavoriteNames", "favoriteName", FieldKind.Text, "Name of a favorite to base the new element on. Its settings are applied first, then the other inputs override them.")
        };

        protected override IReadOnlyList<Field> Fields => FieldDefinitions;

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMeshes;

        public override Guid ComponentGuid =>
            new Guid("e46f531e-4b0c-42f5-8819-2606384742c5");
    }
}
