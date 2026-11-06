using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class ReorderDesignOptionsInSetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReorderDesignOptionsInSet";

        public ReorderDesignOptionsInSetComponent()
            : base(
                "ReorderDesignOptionsInSet",
                "Move the given design options before another design option of the same set or to the end of their set, in the order they are given. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options to move within their set.");

            InGenerics(
                "BeforeDesignOptionGuids",
                "Identifier of the design option of the same set to move each design option before (input only 1 to move all before the same design option). " +
                "Leave it empty, or an item null, to move the design options to the end of their set.");

            SetOptionality(1);
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

            da.TryGetList(
                1,
                out List<GH_ObjectWrapper> beforeWrappers);

            if (beforeWrappers.Count > 1 &&
                beforeWrappers.Count != designOptionWrappers.Count)
            {
                this.AddError(
                    "The size of the input BeforeDesignOptionGuids must be 0, 1 or equal to the size of the input DesignOptionGuids.");
                return;
            }

            var input = new ReorderDesignOptionsInSetParameters
            {
                DesignOptions = new List<ReorderDesignOptionInSetParameter>()
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

                DesignOptionGuid beforeDesignOptionId = null;
                if (beforeWrappers.Count > 0)
                {
                    var beforeWrapper =
                        beforeWrappers[beforeWrappers.Count == 1 ? 0 : i];
                    if (beforeWrapper?.Value != null)
                    {
                        beforeDesignOptionId =
                            GuidObject<DesignOptionGuid>.CreateFromWrapper(beforeWrapper);
                        if (beforeDesignOptionId == null)
                        {
                            this.AddError(
                                "Invalid design option identifier in the BeforeDesignOptionGuids input.");
                            return;
                        }
                    }
                }

                input.DesignOptions.Add(
                    new ReorderDesignOptionInSetParameter
                    {
                        DesignOptionId = designOptionId,
                        BeforeDesignOptionId = beforeDesignOptionId
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
            new Guid("8f4b2d6a-3c1e-4a7f-b9d5-e60c2a8f1b74");
    }
}
