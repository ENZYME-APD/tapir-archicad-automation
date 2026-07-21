using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLibrariesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLibraries";

        public GetLibrariesComponent()
            : base(
                "GetLibraries",
                "Get the list of loaded libraries.",
                GroupNames.Library)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each library.");

            OutTexts(
                "Paths",
                "Path of each library.");

            OutTexts(
                "Types",
                "Type of each library.");

            OutBooleans(
                "Available",
                "True if the library is available.");

            OutBooleans(
                "ReadOnly",
                "True if the library is read-only.");

            OutTexts(
                "TwServerUrls",
                "Teamwork server URL of each library.");

            OutTexts(
                "UrlWebLibraries",
                "Web library URL of each library.");
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

            var names = new List<object>();
            var paths = new List<object>();
            var types = new List<object>();
            var available = new List<object>();
            var readOnly = new List<object>();
            var twServerUrls = new List<object>();
            var urlWebLibraries = new List<object>();

            foreach (var item in JsonOutputHelp.Items(response, "libraries"))
            {
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                paths.Add(JsonOutputHelp.Scalar(item, "path"));
                types.Add(JsonOutputHelp.Scalar(item, "type"));
                available.Add(JsonOutputHelp.Scalar(item, "available"));
                readOnly.Add(JsonOutputHelp.Scalar(item, "readOnly"));
                twServerUrls.Add(JsonOutputHelp.Scalar(item, "twServerUrl"));
                urlWebLibraries.Add(JsonOutputHelp.Scalar(item, "urlWebLibrary"));
            }

            da.SetDataList(0, names);
            da.SetDataList(1, paths);
            da.SetDataList(2, types);
            da.SetDataList(3, available);
            da.SetDataList(4, readOnly);
            da.SetDataList(5, twServerUrls);
            da.SetDataList(6, urlWebLibraries);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetLibraries;

        public override Guid ComponentGuid =>
            new Guid("d5a0af65-47c5-43ba-b6c5-9900feedce9d");
    }
}
