using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByName : ArchicadAccessorComponent
    {
        public static string Doc => "Finds a property by group name and name.";
        public override string CommandName => "GetAllProperties";

        public FindPropertyByName()
            : base(
                "Property by Name",
                "PropertyByName",
                Doc,
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Group name",
                "Property group name to find.");

            InText(
                "Property name",
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
            var propertyGroupName = "";
            var propertyName = "";
            if (!da.GetData(
                    0,
                    ref propertyGroupName) || !da.GetData(
                    1,
                    ref propertyName))
            {
                return;
            }

            if (!GetConvertedResponse(
                    CommandName,
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
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Property is not found.");
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