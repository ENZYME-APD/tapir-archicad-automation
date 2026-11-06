using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class RenameDesignOptionSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameDesignOptionSets";

        public RenameDesignOptionSetsComponent()
            : base(
                "RenameDesignOptionSets",
                "Change the name of the given design option sets. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionSetGuids",
                "Identifiers of the design option sets to rename.");

            InTexts(
                "Names",
                "New name of each design option set.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each design option set (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> setWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (names.Count != setWrappers.Count)
            {
                this.AddError(
                    "The size of the inputs DesignOptionSetGuids and Names must be equal.");
                return;
            }

            var input = new RenameDesignOptionSetsParameters
            {
                DesignOptionSets = new List<RenameDesignOptionSetParameter>()
            };

            for (var i = 0; i < setWrappers.Count; i++)
            {
                var setId =
                    GuidObject<DesignOptionSetGuid>.CreateFromWrapper(setWrappers[i]);
                if (setId == null)
                {
                    this.AddError(
                        "Invalid design option set identifier in the DesignOptionSetGuids input.");
                    return;
                }

                if (string.IsNullOrEmpty(names[i]))
                {
                    this.AddError(
                        "Every item of the Names input needs a name.");
                    return;
                }

                input.DesignOptionSets.Add(
                    new RenameDesignOptionSetParameter
                    {
                        DesignOptionSetId = setId,
                        Name = names[i]
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameDesignOptionSets;

        public override Guid ComponentGuid =>
            new Guid("2905894f-4153-42cd-bc52-8c87b8b975e7");
    }
}
