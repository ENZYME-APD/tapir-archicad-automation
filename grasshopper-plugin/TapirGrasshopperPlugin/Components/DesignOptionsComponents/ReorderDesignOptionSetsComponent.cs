using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class ReorderDesignOptionSetsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReorderDesignOptionSets";

        public ReorderDesignOptionSetsComponent()
            : base(
                "ReorderDesignOptionSets",
                "Move the given design option sets before another set or to the end of the list, in the order they are given. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionSetGuids",
                "Identifiers of the design option sets to move.");

            InGenerics(
                "BeforeDesignOptionSetGuids",
                "Identifier of the set to move each set before (input only 1 to move all sets before the same set). " +
                "Leave it empty, or an item null, to move the sets to the end.");

            SetOptionality(1);
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

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> beforeWrappers);

            if (beforeWrappers.Count > 1 &&
                beforeWrappers.Count != setWrappers.Count)
            {
                this.AddError(
                    "The size of the input BeforeDesignOptionSetGuids must be 0, 1 or equal to the size of the input DesignOptionSetGuids.");
                return;
            }

            var input = new ReorderDesignOptionSetsParameters
            {
                DesignOptionSets = new List<ReorderDesignOptionSetParameter>()
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

                DesignOptionSetGuid beforeSetId = null;
                if (beforeWrappers.Count > 0)
                {
                    var beforeWrapper =
                        beforeWrappers[beforeWrappers.Count == 1 ? 0 : i];
                    if (beforeWrapper?.Value != null)
                    {
                        beforeSetId =
                            GuidObject<DesignOptionSetGuid>.CreateFromWrapper(beforeWrapper);
                        if (beforeSetId == null)
                        {
                            this.AddError(
                                "Invalid design option set identifier in the BeforeDesignOptionSetGuids input.");
                            return;
                        }
                    }
                }

                input.DesignOptionSets.Add(
                    new ReorderDesignOptionSetParameter
                    {
                        DesignOptionSetId = setId,
                        BeforeDesignOptionSetId = beforeSetId
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                input,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveDesignOptionsToAnotherSet;

        public override Guid ComponentGuid =>
            new Guid("c2e7a1f4-8d3b-4e59-a6c0-1f9b4d7e2a63");
    }
}
