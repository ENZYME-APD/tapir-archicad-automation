using System;

namespace TapirGrasshopperPlugin.Components.DesignToolsComponents
{
    // The Window tool, from ToolSettings/Window.json; see OpeningToolComponent.
    public class WindowsComponent : OpeningToolComponent
    {
        protected override string ToolName => "Window";
        protected override string ElementNoun => "window";

        protected override string CreateCommand => "CreateWindows";
        protected override string CreateArrayKey => "windowsData";
        protected override string ModifyCommand => "ModifyWindows";
        protected override string ModifyArrayKey => "windowsWithDetails";

        public WindowsComponent()
            : base(
                "Windows",
                "Reads (Refresh), writes (Execute) and creates (Create) windows, with the settings of the " +
                "Window Settings dialog as one JSON object per dialog panel. A new window is placed in OwnerWall " +
                "at CenterOffset; it starts from the Window tool defaults, or from FavoriteName, and the given " +
                "settings override them.")
        {
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateWindows;

        public override Guid ComponentGuid =>
            new Guid("8e41c2b7-3a9d-4f60-b5e2-7c1d9a0f6e53");
    }
}
