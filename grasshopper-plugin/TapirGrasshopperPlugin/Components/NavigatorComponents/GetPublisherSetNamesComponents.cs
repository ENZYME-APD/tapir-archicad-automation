using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class PublisherSetNamesObj
    {
        [JsonProperty("publisherSetNames")]
        public List<string> PublisherSetNames;
    }

    public class GetPublisherSetNames : ArchicadAccessorComponent
    {
        public GetPublisherSetNames()
            : base(
                "PublisherSetNames",
                "PublisherSetNames",
                "Get names of the publisher sets.",
                "Navigator")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "PublisherSetNames",
                "PublisherSetNames",
                "List of names of the publisher sets.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var response = SendArchicadCommand(
                "GetPublisherSetNames",
                null);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var obj = response.Result.ToObject<PublisherSetNamesObj>();
            DA.SetDataList(
                0,
                obj.PublisherSetNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.PublisherSetNames;

        public override Guid ComponentGuid =>
            new Guid("446d3bca-f817-4c90-b163-44da5197e707");
    }
}