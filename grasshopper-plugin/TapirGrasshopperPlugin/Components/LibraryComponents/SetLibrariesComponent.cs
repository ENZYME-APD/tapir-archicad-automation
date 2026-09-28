using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class SetLibrariesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetLibraries";

        public SetLibrariesComponent()
            : base(
                "SetLibraries",
                "Replace the loaded libraries with the given ones. Give local library folders or container files by absolute path.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Paths",
                "Absolute path of each library folder or container file.");
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var paths = new List<string>();
            da.GetDataList(0, paths);

            var libraries = new JArray();
            foreach (var path in paths)
            {
                if (string.IsNullOrWhiteSpace(path))
                {
                    this.AddError("The input Paths contains an empty path.");
                    return;
                }
                libraries.Add(new JObject { ["path"] = path });
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["libraries"] = libraries },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLibraries;

        public override Guid ComponentGuid =>
            new Guid("94ba3208-5c0b-12e6-b79b-b9cbbc9c139f");
    }
}
