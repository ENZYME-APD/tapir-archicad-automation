using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectNotesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetProjectNotes";

        public SetProjectNotesComponent()
            : base(
                "SetProjectNotes",
                "Changes the Project Notes of the project (File > Info > Project Notes). Only the given fields are " +
                "changed; texts longer than the field Archicad stores are truncated.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            for (var i = 0; i < ProjectNotesFields.Count; i++)
            {
                this.AddOptionalText(
                    ProjectNotesFields.Name(i),
                    ProjectNotesFields.Description(i) + " Unconnected means unchanged.");
            }
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            for (var i = 0; i < ProjectNotesFields.Count; i++)
            {
                da.TryAddOptional<string>(
                    i,
                    parameters,
                    ProjectNotesFields.Key(i));
            }

            if (parameters.Count == 0)
            {
                this.AddError("Give at least one field to change.");
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            da.SetData(
                0,
                ProjectComponentHelp.ExecutionResultMessage(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetProjectNotes;

        public override Guid ComponentGuid =>
            new Guid("3730f4c2-f2db-42d4-97d0-0a7d3bb9bdae");
    }
}
