using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class SetDefaultDesignOptionComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetDefaultDesignOption";

        public SetDefaultDesignOptionComponent()
            : base(
                "SetDefaultDesignOption",
                "Set the given design option as the default, or the Main Model when no design option is given. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "DesignOptionGuid",
                "Identifier of the design option to set as the default. Leave it empty to set the Main Model as the default.");

            SetOptionality(0);
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
            var input = new SetDefaultDesignOptionParameters();

            if (da.TryGet(
                    0,
                    out GH_ObjectWrapper wrapper))
            {
                input.DesignOptionId =
                    GuidObject<DesignOptionGuid>.CreateFromWrapper(wrapper);
                if (input.DesignOptionId == null)
                {
                    this.AddError(
                        "Invalid design option identifier in the DesignOptionGuid input.");
                    return;
                }
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError($"Failed execution: {response.Message()}");
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetActiveDesignOptionsInCombinations;

        public override Guid ComponentGuid =>
            new Guid("6b0d3c8e-4f2a-4b6e-9c1d-7a5e2f8b3d41");
    }
}
