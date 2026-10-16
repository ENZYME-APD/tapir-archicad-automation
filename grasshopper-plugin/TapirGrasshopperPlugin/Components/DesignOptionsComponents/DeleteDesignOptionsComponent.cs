using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class DeleteDesignOptionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteDesignOptions";

        public DeleteDesignOptionsComponent()
            : base(
                "DeleteDesignOptions",
                "Delete the given design options. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option (empty when it was deleted successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var input = DesignOptionsObject.ConvertFrom(wrappers);
            if (input == null)
            {
                this.AddError(
                    "Invalid design option identifier in the DesignOptionGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("c16f1a9f-c81a-44a9-a566-defa564eaa83");
    }
}
