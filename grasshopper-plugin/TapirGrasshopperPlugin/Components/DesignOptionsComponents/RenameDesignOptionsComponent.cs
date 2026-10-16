using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class RenameDesignOptionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameDesignOptions";

        public RenameDesignOptionsComponent()
            : base(
                "RenameDesignOptions",
                "Change the name and/or the string id of the given design options. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options to rename.");

            InTexts(
                "Names",
                "New name of each design option. Optional: leave it unconnected, or give an empty text for an item, to keep the current name.");

            InTexts(
                "Ids",
                "New string id of each design option. Optional: leave it unconnected, or give an empty text for an item, to keep the current id.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> designOptionWrappers))
            {
                return;
            }

            var names = da.GetOptionals<string>(1);
            var ids = da.GetOptionals<string>(2);

            if (names.Count == 0 &&
                ids.Count == 0)
            {
                this.AddError(
                    "Give at least one of the inputs Names and Ids.");
                return;
            }

            if ((names.Count != 0 && names.Count != designOptionWrappers.Count) ||
                (ids.Count != 0 && ids.Count != designOptionWrappers.Count))
            {
                this.AddError(
                    "The size of the inputs Names and Ids must be equal to the size of the input DesignOptionGuids (or 0 to keep them all).");
                return;
            }

            var input = new RenameDesignOptionsParameters
            {
                DesignOptions = new List<RenameDesignOptionParameter>()
            };

            for (var i = 0; i < designOptionWrappers.Count; i++)
            {
                var designOptionId =
                    GuidObject<DesignOptionGuid>.CreateFromWrapper(designOptionWrappers[i]);
                if (designOptionId == null)
                {
                    this.AddError(
                        "Invalid design option identifier in the DesignOptionGuids input.");
                    return;
                }

                // An omitted field keeps the current value, so an empty text
                // means "keep" instead of an empty name or id.
                input.DesignOptions.Add(
                    new RenameDesignOptionParameter
                    {
                        DesignOptionId = designOptionId,
                        Name = names.Count == 0 || string.IsNullOrEmpty(names[i])
                            ? null
                            : names[i],
                        Id = ids.Count == 0 || string.IsNullOrEmpty(ids[i])
                            ? null
                            : ids[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("fdfc3420-9849-4666-8e0d-18d2efc81178");
    }
}
