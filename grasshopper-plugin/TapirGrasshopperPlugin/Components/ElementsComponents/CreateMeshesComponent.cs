using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class CreateMeshesComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "CreateMeshes";

        public CreateMeshesComponent()
            : base(
                "CreateMeshes",
                "Create Mesh elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"level\":0,\"polygonCoordinates\":[{\"x\":0,\"y\":0,\"z\":0},...]}.",
                GroupNames.Elements,
                "meshesData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateMeshes;

        public override Guid ComponentGuid =>
            new Guid("e46f531e-4b0c-42f5-8819-2606384742c5");
    }
}
