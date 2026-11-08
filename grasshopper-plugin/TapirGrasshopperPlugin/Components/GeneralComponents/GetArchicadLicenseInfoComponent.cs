using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetArchicadLicenseInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetArchicadLicenseInfo";

        public GetArchicadLicenseInfoComponent()
            : base(
                "GetArchicadLicenseInfo",
                "Retrieves the license the running Archicad runs on: its type, the raw protection mode with its " +
                "bits by name, and the highest Software Service Agreement (SSA) level of the license, so a " +
                "definition can warn before it uses a feature the license lacks.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "LicenseType",
                "The kind of license Archicad runs on: Full, Education, Trial, Demo, Start or BIMcloudTeamClient.");

            OutInteger(
                "ProtectionMode",
                "The raw protection mode bit mask reported by Archicad.");

            OutTexts(
                "LicenseFlags",
                "The bits set in ProtectionMode, by name (e.g. Network, Teamwork, Education, Start, Solo, " +
                "LiveSubscription).");

            OutText(
                "SSALevel",
                "The highest Software Service Agreement level of the license: NoSSA, SSA or Forward. " +
                "Empty in Archicad 25, which cannot report it.");
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
                response["licenseType"]?.ToString());

            da.SetData(
                1,
                (int?)response["protectionMode"]);

            var flags = new List<string>();
            if (response["licenseFlags"] is JArray flagArray)
            {
                foreach (var flag in flagArray)
                {
                    flags.Add(flag.ToString());
                }
            }

            da.SetDataList(
                2,
                flags);

            da.SetData(
                3,
                response["ssaLevel"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetArchicadLicenseInfo;

        public override Guid ComponentGuid =>
            new Guid("aab4a08b-498a-47a6-95f0-05dbf86f9faa");
    }
}
