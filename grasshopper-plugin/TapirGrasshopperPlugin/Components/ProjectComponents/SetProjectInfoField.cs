using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectInfoFieldsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SetProjectInfoField";

        public SetProjectInfoFieldsComponent()
            : base(
                "SetProjectInfoField",
                "Set a project info field to a requested value.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                nameof(ProjectInfoField.ProjectInfoId),
                "Guid to select the field to set.");

            InText(
                nameof(ProjectInfoField.ProjectInfoValue),
                "Value to set the field to.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out string id)) { return; }

            if (!da.TryGetItem(
                    1,
                    out string value)) { return; }

            SetArchiCadValues(
                CommandName,
                new ProjectInfoField
                {
                    ProjectInfoId = id, ProjectInfoValue = value
                });
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("37d5a284-c0bb-4d3d-80a6-35a4f5c7d356");
    }
}