using Grasshopper.Kernel;
using System;
using System.Diagnostics;

namespace Tapir.Components.Utilities
{
    public class TapirOpenDatabaseComponent : ButtonComponent
    {
        /// <summary>
        /// Initializes a new instance of the TapirOpenDatabaseComponent class.
        /// </summary>
        public TapirOpenDatabaseComponent()
            : base("Open Website Button", "Prop_DB", 
                  "Opens a specified URL in the default browser", "Utilities")
        {
            CapsuleButtonText = "Open Database";
        }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddBooleanParameter("Run", "R", "Set to true to open the website", GH_ParamAccess.item, false);
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            // No outputs
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object is used to retrieve from inputs and store in outputs.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            bool run = false;
            DA.GetData(0, ref run);

            if (run)
            {
                OpenWebsite("https://app.teable.io/invite?invitationId=invgOX6hmfzc7LrBfsM&invitationCode=8dc6c46ad990e79239f36e071dc1264fc378581b86c109b268d6ca9cdc6fa544");
            }

            // Update component message
            this.Message = "Properties GUID\nV1.0";
        }

        /// <summary>
        /// Opens the specified URL in the default browser.
        /// </summary>
        private void OpenWebsite(string url)
        {
            try
            {
                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = url,
                    UseShellExecute = true
                };
                Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                this.AddRuntimeMessage(GH_RuntimeMessageLevel.Error, $"Failed to open URL: {ex.Message}");
            }
        }

        /// <summary>
        /// Called when the button on the component is pressed.
        /// </summary>
        public override void OnCapsuleButtonPressed()
        {
            OpenWebsite("https://app.teable.io/invite?invitationId=invgOX6hmfzc7LrBfsM&invitationCode=8dc6c46ad990e79239f36e071dc1264fc378581b86c109b268d6ca9cdc6fa544");
        }

        /// <summary>
        /// Provides an Icon for the component.
        /// </summary>
        protected override System.Drawing.Bitmap Icon
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// Gets the unique ID for this component. Do not change this ID after release.
        /// </summary>
        public override Guid ComponentGuid
        {
            get { return new Guid("A8B11162-627B-455B-B9C0-963E76B36DC8"); }
        }
    }
}
