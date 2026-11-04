using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.LibraryComponents
{
    public class OpenLibraryPartInEditorComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "OpenLibraryPartInEditor";

        public OpenLibraryPartInEditorComponent()
            : base(
                "OpenLibraryPartInEditor",
                "Opens a loaded library part, given by its fileName or documentName as returned by " +
                "GetAvailableLibraryParts, in Archicad's library part editor. Available from Archicad 27.",
                GroupNames.Library)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "Name",
                "The fileName (with extension, e.g. 'My Object.gsm') or the documentName of the loaded library part, " +
                "as returned by GetAvailableLibraryParts.");
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
            if (!da.TryGet(0, out string name))
            {
                return;
            }

            if (string.IsNullOrWhiteSpace(name))
            {
                this.AddError("The input Name is empty.");
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new JObject { ["name"] = name },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateLibraryPart;

        public override Guid ComponentGuid =>
            new Guid("6204f413-69be-4d9d-a6e2-2a830c04b464");
    }
}
