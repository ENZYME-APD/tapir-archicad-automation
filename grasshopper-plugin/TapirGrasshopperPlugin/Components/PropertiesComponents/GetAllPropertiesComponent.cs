using Grasshopper.Kernel;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Properties;

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
            OutGenerics("PropertyIds");
            OutTexts("GroupNames");
            OutTexts("PropertyNames");

            OutTexts(
                "FullNames",
                "Full names containing the joined group and property name.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    ToAddOn,
                    JHelp.Deserialize<AllProperties>,
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
                response.Properties.Select(x => StringHelp.Join(
                    x.PropertyGroupName,
                    x.PropertyName)));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllProperties;

        public override Guid ComponentGuid =>
            new Guid("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}