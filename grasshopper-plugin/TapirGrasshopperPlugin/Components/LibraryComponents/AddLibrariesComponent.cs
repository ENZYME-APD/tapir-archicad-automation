using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class AddLibrariesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "AddLibraries";

        public AddLibrariesComponent()
            : base(
                "AddLibraries",
                "Add the given libraries to the loaded ones. Give local library folders or container files by absolute path.",
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
            Properties.Resources.AddLibraries;

        public override Guid ComponentGuid =>
            new Guid("f2cb88d4-fc80-a599-2132-8fcca94fc620");
    }
}
