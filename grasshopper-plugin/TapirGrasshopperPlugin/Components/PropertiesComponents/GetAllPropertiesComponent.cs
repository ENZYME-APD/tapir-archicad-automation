using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetAllPropertiesComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Get all properties.";

        public override string CommandName => "GetAllProperties";

        public GetAllPropertiesComponent()
            : base(
                "All Properties",
                "AllProperties",
                Doc,
                GroupNames.Properties)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "PropertyGuid",
                "PropertyGuid",
                "Property Guid.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "GroupName",
                "GroupName",
                "Property group name.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "PropertyName",
                "PropertyName",
                "Property name.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "FullName",
                "FullName",
                "Full name containing the group and the property name.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
                    CommandName,
                    out AllProperties response))
            {
                return;
            }

            var propertyIds = new List<PropertyIdObj>();
            var propertyGroupNames = new List<string>();
            var propertyNames = new List<string>();
            var fullNames = new List<string>();

            foreach (var detail in response.Properties)
            {
                propertyIds.Add(detail.PropertyId);
                propertyGroupNames.Add(detail.PropertyGroupName);
                propertyNames.Add(detail.PropertyName);
                fullNames.Add(
                    ArchicadUtils.JoinNames(
                        detail.PropertyGroupName,
                        detail.PropertyName));
            }

            da.SetDataList(
                0,
                propertyIds);
            da.SetDataList(
                1,
                propertyGroupNames);
            da.SetDataList(
                2,
                propertyNames);
            da.SetDataList(
                3,
                fullNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllProperties;

        public override Guid ComponentGuid =>
            new Guid("79f924e4-5b26-4efe-bfaf-0e82f9bb3821");
    }
}