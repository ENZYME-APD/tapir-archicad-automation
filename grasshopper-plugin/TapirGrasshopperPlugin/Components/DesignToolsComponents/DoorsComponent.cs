using System;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Door tool, from ToolSettings/Door.json; see OpeningToolComponent.
    public class DoorsComponent : OpeningToolComponent
    {
        protected override string ToolName => "Door";
        protected override string ElementNoun => "door";

        protected override string CreateCommand => "CreateDoors";
        protected override string CreateArrayKey => "doorsData";
        protected override string ModifyCommand => "ModifyDoors";
        protected override string ModifyArrayKey => "doorsWithDetails";

        public DoorsComponent()
            : base(
                "Doors",
                "Reads (Refresh), writes (Execute) and creates (Create) doors, with the settings of the " +
                "Door Settings dialog as one JSON object per dialog panel. A new door is placed in OwnerWall at " +
                "CenterOffset; it starts from the Door tool defaults, or from FavoriteName, and the given " +
                "settings override them.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDoors;

        public override Guid ComponentGuid =>
            new Guid("5d3c6a1e-7f42-4b8e-9c1d-2a6e0f4b8d37");
    }
}
