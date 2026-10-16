using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class DeleteDesignOptionSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "DeleteDesignOptionSets";

        public DeleteDesignOptionSetsComponent()
            : base(
                "DeleteDesignOptionSets",
                "Delete the given design option sets. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionSetGuids",
                "Identifiers of the design option sets to delete.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option set (empty when it was deleted successfully).");
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

            var input = DesignOptionSetsObject.ConvertFrom(wrappers);
            if (input == null)
            {
                this.AddError(
                    "Invalid design option set identifier in the DesignOptionSetGuids input.");
                return;
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.DeleteDesignOptionSets;

        public override Guid ComponentGuid =>
            new Guid("1c1f4619-fcbd-4a56-8960-6a12677f25e3");
    }
}
