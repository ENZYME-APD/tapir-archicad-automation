using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class CreateDesignOptionSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateDesignOptionSets";

        public CreateDesignOptionSetsComponent()
            : base(
                "CreateDesignOptionSets",
                "Create new design option sets with the given names. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the design option sets to create.");
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

            var input = new CreateDesignOptionSetsParameters
            {
                DesignOptionSets = names
            };

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDesignOptionSets;

        public override Guid ComponentGuid =>
            new Guid("0a082dc5-df6f-4386-a4b8-9b6162b1579b");
    }
}
