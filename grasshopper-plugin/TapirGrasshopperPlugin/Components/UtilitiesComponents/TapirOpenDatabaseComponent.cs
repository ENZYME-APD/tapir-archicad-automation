using Grasshopper.Kernel;
using System;
using System.Diagnostics;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class TapirOpenDatabaseComponent : ButtonComponent
    {
        public static string Doc =>
            "Opens a specified URL in the default browser.";

        static public string WebsiteURL =
            "https://app.teable.io/invite?invitationId=invgOX6hmfzc7LrBfsM&invitationCode=8dc6c46ad990e79239f36e071dc1264fc378581b86c109b268d6ca9cdc6fa544";

        public TapirOpenDatabaseComponent()
            : base(
                "Open Website Button",
                "Prop_DB",
                Doc,
                GroupNames.Utilities,
                "Open Database",
                80)
        {
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            // Update component message
            Message = "Properties GUID\nV1.0";
        }

        private void OpenWebsite(
            string url)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = url, UseShellExecute = true
                };
                Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    $"Failed to open URL: {ex.Message}");
            }
        }

        public override void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            OpenWebsite(WebsiteURL);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.OpenWebsiteButton;

        public override Guid ComponentGuid =>
            new Guid("A8B11162-627B-455B-B9C0-963E76B36DC8");
    }
}