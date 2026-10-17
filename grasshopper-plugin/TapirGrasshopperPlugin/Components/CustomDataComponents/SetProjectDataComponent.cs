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
                "Store text values under keys in the project, as data of the Tapir add-on: " +
                "saved with the project, shown nowhere in the Archicad UI. " +
                "A key that already exists gets the new value, other keys are left as they are.",
                GroupNames.CustomData)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Keys",
                "Keys to store the values under. Prefix them (for example 'myscript.lastSync') " +
                "so that different scripts do not overwrite each other's data.");

            InTexts(
                "Values",
                "Value to store under each key (input only 1 to use the same value for all keys).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each key (empty when it was stored).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> keys))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> values))
            {
                return;
            }

            if (values.Count != 1 &&
                values.Count != keys.Count)
            {
                this.AddError(
                    "The count of Values must be 1 or the same as the count of Keys.");
                return;
            }

            if (keys.Exists(string.IsNullOrEmpty))
            {
                this.AddError("Every key must be a non-empty text.");
                return;
            }

            var input = new SetProjectDataParameters
            {
                ProjectData = new List<KeyValuePairObj>()
            };

            for (var i = 0; i < keys.Count; i++)
            {
                input.ProjectData.Add(
                    new KeyValuePairObj
                    {
                        Key = keys[i],
                        Value = values.Count == 1
                            ? values[0]
                            : values[i]
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
