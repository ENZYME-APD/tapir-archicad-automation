using System;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ModifyMeshesComponent : JsonItemsExecutorComponent
    {
        public override string CommandName => "ModifyMeshes";

        public ModifyMeshesComponent()
            : base(
                "ModifyMeshes",
                "Modify the attributes of Mesh elements based on the given parameters. Each input item is a JSON object matching the command's documented item schema, e.g. {\"elementId\":{\"guid\":\"...\"},...}.",
                GroupNames.Elements,
                "meshesData",
                "ItemsData",
                "One JSON object per item, matching the command's documented item schema (see component description).")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyMeshes;

        public override Guid ComponentGuid =>
            new Guid("f1808d03-c95c-4b54-b96a-cfc1b30a1562");
    }
}
