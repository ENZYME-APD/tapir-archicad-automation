using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectInfoFieldsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SetProjectInfoField";

        public SetProjectInfoFieldsComponent()
            : base(
                "SetProjectInfoFields",
                "Set a project info field to a requested value.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                nameof(ProjectInfoField.ProjectInfoId) + "s",
                "Guids of the targeted project info fields.");

            InTexts(
                nameof(ProjectInfoField.ProjectInfoValue) + "s",
                "Values to set the fields to.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItems(
                    0,
                    out List<string> ids))
            {
                return;
            }


            if (!da.TryGetItems(
                    1,
                    out List<string> values))
            {
                return;
            }

            if (ids.Count != values.Count)
            {
                this.AddError("Id to Value count mismatch!");
            }

            if (ids.Count != ids.Distinct().Count())
            {
                this.AddError("Duplicate input ids!");
            }

            foreach (var field in ids.Zip(
                         values,
                         (
                             x,
                             y) => new ProjectInfoField(
                             x,
                             y)))
            {
                SetArchiCadValues(
                    CommandName,
                    field);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("37d5a284-c0bb-4d3d-80a6-35a4f5c7d356");
    }
}