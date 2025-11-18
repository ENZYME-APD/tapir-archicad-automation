using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class FindPropertyByGuidComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Finds a property by guid.";
        public override string CommandName => "GetAllProperties";

        public FindPropertyByGuidComponent()
            : base(
                "Property by Guid",
                "PropertyByGuid",
                Doc,
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            AddText(
                "PropertyGuid",
                "Property guid to find.");
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "PropertyId",
                "PropertyId",
                "Found property id.",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            string propertyGuid = "";
            if (!da.GetData(
                    0,
                    ref propertyGuid))
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
            propertyGuid = propertyGuid.ToLower();

            foreach (PropertyDetailsObj detail in response.Properties)
            {
                if (detail.PropertyId.Guid.ToLower() == propertyGuid)
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
            Properties.Resources.PropertyByGuid;

        public override Guid ComponentGuid =>
            new("d7f26316-9d62-48b4-854f-ab3d79db1cbf");
    }
}