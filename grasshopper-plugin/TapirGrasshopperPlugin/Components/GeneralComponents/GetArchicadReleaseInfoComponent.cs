using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetArchicadReleaseInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetArchicadReleaseInfo";

        public GetArchicadReleaseInfoComponent()
            : base(
                "GetArchicadReleaseInfo",
                "Retrieves the main version, release version and build number of the running Archicad, with the caption of its window.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutInteger(
                "MainVersion",
                "The main version of the running Archicad, e.g. 29.");

            OutInteger(
                "ReleaseVersion",
                "The release (update) version of the running Archicad.");

            OutInteger(
                "BuildNumber",
                "The build number of the running Archicad.");

            OutText(
                "Caption",
                "The caption of the Archicad application window. Empty when Archicad does not report it.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (int?)response["mainVersion"]);

            da.SetData(
                1,
                (int?)response["releaseVersion"]);

            da.SetData(
                2,
                (int?)response["buildNumber"]);

            da.SetData(
                3,
                response["caption"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetArchicadReleaseInfo;

        public override Guid ComponentGuid =>
            new Guid("df6b209c-adec-43cb-9bc4-f5f13f63dd1a");
    }
}
