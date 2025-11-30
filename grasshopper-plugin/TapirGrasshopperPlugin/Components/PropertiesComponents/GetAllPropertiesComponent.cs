using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllProperties";

        public GetAllPropertiesComponent()
            : base(
                "AllProperties",
                "Get all properties.",
                GroupNames.Properties)
        {
        }

        protected override void AddOutputs()
        {
            OutGeneric("PropertyGuid");
            OutTexts("GroupName");
            OutTexts("PropertyName");

            OutTexts(
                "FullName",
                "Full name containing the group and the property name.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedResponse(
                    CommandName,
                    out AllProperties response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.Properties.Select(x => x.PropertyId));

            da.SetDataList(
                1,
                response.Properties.Select(x => x.PropertyGroupName));

            da.SetDataList(
                2,
                response.Properties.Select(x => x.PropertyName));

            da.SetDataList(
                3,
                response.Properties.Select(x => ArchicadUtils.JoinNames(
                    x.PropertyGroupName,
                    x.PropertyName)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllProperties;

        public override Guid ComponentGuid =>
            new Guid("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}