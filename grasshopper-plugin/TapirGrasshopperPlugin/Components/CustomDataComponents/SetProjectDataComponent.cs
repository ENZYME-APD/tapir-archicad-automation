using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.CustomData;

namespace TapirGrasshopperPlugin.Components.CustomDataComponents
{
    public class SetProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetProjectData";

        public SetProjectDataComponent()
            : base(
                "SetProjectData",
                "Store text (for example JSON) in the project under names: " +
                "saved with the project, shown nowhere in the Archicad UI. " +
                "A name that already exists gets the new content, other names are left as they are.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names to store the contents under. Prefix them with the name of your tool " +
                "(for example 'my-tool/settings') so that different scripts do not overwrite each other's data.");

            InTexts(
                "Contents",
                "Content to store under each name (input only 1 to use the same content for all names).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each name (empty when it was stored).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> contents))
            {
                return;
            }

            if (contents.Count != 1 &&
                contents.Count != names.Count)
            {
                this.AddError(
                    "The count of Contents must be 1 or the same as the count of Names.");
                return;
            }

            if (names.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every name must be a non-empty text.");
                return;
            }

            var input = new SetProjectDataParameters
            {
                Values = new List<ProjectDataValue>()
            };

            for (var i = 0; i < names.Count; i++)
            {
                input.Values.Add(
                    new ProjectDataValue
                    {
                        Name = names[i],
                        Content = contents.Count == 1
                            ? contents[0]
                            : contents[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        // The project data has no icon of its own yet; it shares the project
        // info one, as the project info field components do among themselves.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ProjectInfo;

        public override Guid ComponentGuid =>
            new Guid("7c1f4a2e-9b3d-4e6f-8a21-5d0c3b7e9f14");
    }
}
