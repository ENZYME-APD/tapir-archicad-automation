using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class NewProjectComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "NewProject";

        public NewProjectComponent()
            : base(
                "NewProject",
                "Start a new, untitled project from the template set in Archicad's " +
                "New Project settings, as File > New does. The unsaved changes of " +
                "the open project are discarded, as with OpenProject.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InBoolean(
                "NewAndReset",
                "Also reset the working environment to the template's, as " +
                "File > New > New and Reset All does, instead of keeping the " +
                "current one as File > New does.",
                false);
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
            var parameters = new JObject
            {
                ["newAndReset"] = da.GetOptional(0, false)
            };

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            da.SetData(0, response.Message());
        }

        // Shares the OpenProject icon until a badged one is drawn for it.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.OpenProject;

        public override Guid ComponentGuid =>
            new Guid("a2f7c3d1-5e84-4b6a-9d13-7c2e8f0b4a61");
    }
}
