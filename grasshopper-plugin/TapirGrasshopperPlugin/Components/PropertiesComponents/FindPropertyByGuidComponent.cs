using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByGuidComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllProperties";

        public FindPropertyByGuidComponent()
            : base(
                "PropertyByGuid",
                "Finds a property by Guid.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "PropertyGuid",
                "Property Guid to find.");
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "PropertyId",
                "Found property Id.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out string propertyGuid))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    out AllProperties response))
            {
                return;
            }

            PropertyDetailsObj found = null;
            propertyGuid = propertyGuid.ToLower();

            foreach (var detail in response.Properties)
            {
                if (detail.PropertyId.Guid.ToLower() == propertyGuid)
                {
                    found = detail;
                    break;
                }
            }

            if (found == null)
            {
                this.AddError("Property is not found.");
            }
            else
            {
                da.SetData(
                    0,
                    found.PropertyId);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PropertyByGuid;

        public override Guid ComponentGuid =>
            new Guid("d7f26316-9d62-48b4-854f-ab3d79db1cbf");
    }
}