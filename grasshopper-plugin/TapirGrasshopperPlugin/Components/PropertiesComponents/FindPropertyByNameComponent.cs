using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByName : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllProperties";

        public FindPropertyByName()
            : base(
                "PropertyByName",
                "Finds a property by group name and name.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "GroupName",
                "Property group name to find.");

            InText(
                "PropertyName",
                "Property name to find.");
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "PropertyGuid",
                "Found property Guid.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string propertyGroupName))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string propertyName))
            {
                return;
            }


            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<AllProperties>,
                    out AllProperties response))
            {
                return;
            }

            PropertyDetailsObj found = null;

            propertyGroupName = propertyGroupName.ToLower();
            propertyName = propertyName.ToLower();

            foreach (var detail in response.Properties)
            {
                if (detail.PropertyGroupName.ToLower() == propertyGroupName &&
                    detail.PropertyName.ToLower() == propertyName)
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
            Properties.Resources.PropertyByName;

        public override Guid ComponentGuid =>
            new Guid("9bb30fb5-9a68-4672-b807-4e35b2d27761");
    }
}