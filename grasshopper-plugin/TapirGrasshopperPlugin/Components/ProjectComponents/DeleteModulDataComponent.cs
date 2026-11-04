using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class DeleteModulDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteModulData";

        public DeleteModulDataComponent()
            : base(
                "DeleteModulData",
                "Deletes the module data with the given names. Deleting module data is not an undoable step.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the module data to delete; the empty name is the default section.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each name (empty when its module data was deleted).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["names"] = JArray.FromObject(names) },
                ToAddOn,
                da);
        }

        // Shares the icon of DeleteProjectData, the other text data saved in the project.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteProjectData;

        public override Guid ComponentGuid =>
            new Guid("b9b61a15-5f78-4fe7-8bbd-1ce9a99193dd");
    }
}
