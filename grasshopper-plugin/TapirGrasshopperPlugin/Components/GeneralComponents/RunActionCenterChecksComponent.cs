using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class RunActionCenterChecksComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RunActionCenterChecks";

        public RunActionCenterChecksComponent()
            : base(
                "RunActionCenterChecks",
                "Runs the given checks of the Action Center (by default the hotlink check). A successful " +
                "result means the check ran, not that nothing was found: Archicad does not report the " +
                "outcome to the caller, it shows the Action Center tab when the check found an issue.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Checks",
                "Optional. The Action Center checks to run, in this order: Hotlinks, All, Libraries, " +
                "Drawings or Licenses. Unconnected means Hotlinks. Hotlinks is available on every Archicad " +
                "version; All, Libraries, Drawings and Licenses only on Archicad 25, later versions return " +
                "an error message for them.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each check (empty when it ran).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var checks = new List<string>();
            foreach (var check in da.GetOptionals<string>(0))
            {
                if (!string.IsNullOrWhiteSpace(check))
                {
                    checks.Add(check.Trim());
                }
            }

            if (checks.Count > 0)
            {
                parameters["checks"] = JArray.FromObject(checks);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        // Shares the icon of UpdateHotlinks until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.UpdateHotlinks;

        public override Guid ComponentGuid =>
            new Guid("856988ea-1f97-4d42-bdf8-5a7a95ed825f");
    }
}
