using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class CreateDesignOptionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateDesignOptions";

        public CreateDesignOptionsComponent()
            : base(
                "CreateDesignOptions",
                "Create new design options with the given parameters. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the design options to create.");

            InTexts(
                "Ids",
                "String ids of the design options to create.");

            InTexts(
                "OwnerSetNames",
                "Names of the owner design option sets.");
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
                    out List<string> ids))
            {
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<string> ownerSetNames))
            {
                return;
            }

            if (names.Count != ids.Count ||
                names.Count != ownerSetNames.Count)
            {
                this.AddError(
                    "The size of the inputs Names, Ids and OwnerSetNames must be equal.");
                return;
            }

            var input = new CreateDesignOptionsParameters
            {
                DesignOptions = new List<CreateDesignOptionParameter>()
            };

            for (var i = 0; i < names.Count; i++)
            {
                input.DesignOptions.Add(
                    new CreateDesignOptionParameter
                    {
                        Name = names[i],
                        Id = ids[i],
                        OwnerSetName = ownerSetNames[i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("ba1b9ef5-5944-4cc1-b578-b58c3a4bdf2c");
    }
}
