using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class GetLastValidEmbeddedLibraryPartNameComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLastValidEmbeddedLibraryPartName";

        public GetLastValidEmbeddedLibraryPartNameComponent()
            : base(
                "GetLastValidEmbeddedLibraryPartName",
                "Gets the name and file path of the last valid library part of the embedded library.",
                GroupNames.Library)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "Name",
                "The name of the last valid library part of the embedded library.");

            OutText(
                "Path",
                "The path of the file of that library part.");
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

            da.SetData(0, response["name"]?.ToString());
            da.SetData(1, response["path"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AddFilesToEmbeddedLibrary;

        public override Guid ComponentGuid =>
            new Guid("2fed952a-e74c-4e09-b139-5f39a3400a6f");
    }
}
