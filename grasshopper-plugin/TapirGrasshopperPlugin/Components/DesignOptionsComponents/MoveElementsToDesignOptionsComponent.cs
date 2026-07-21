using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class MoveElementsToDesignOptionsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveElementsToDesignOptions";

        public MoveElementsToDesignOptionsComponent()
            : base(
                "MoveElementsToDesignOptions",
                "Move the given elements into the given design options. " +
                "Use a NULL guid design option to remove the element from any design option and move it to the main model. " +
                "Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to move.");

            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the target design options (input only 1 to move all elements into the same design option).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> designOptionWrappers))
            {
                return;
            }

            if (designOptionWrappers.Count != 1 &&
                designOptionWrappers.Count != elementWrappers.Count)
            {
                this.AddError(
                    "The size of the input DesignOptionGuids must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var input = new MoveElementsToDesignOptionsParameters
            {
                ElementDesignOptionPairs = new List<ElementDesignOptionPair>()
            };

            for (var i = 0; i < elementWrappers.Count; i++)
            {
                var elementId = GuidObject<ElementGuid>.CreateFromWrapper(elementWrappers[i]);
                if (elementId == null)
                {
                    this.AddError(
                        "Invalid element identifier in the ElementGuids input.");
                    return;
                }

                var designOptionWrapper =
                    designOptionWrappers[designOptionWrappers.Count == 1 ? 0 : i];
                var designOptionId =
                    GuidObject<DesignOptionGuid>.CreateFromWrapper(designOptionWrapper);
                if (designOptionId == null)
                {
                    this.AddError(
                        "Invalid design option identifier in the DesignOptionGuids input.");
                    return;
                }

                input.ElementDesignOptionPairs.Add(
                    new ElementDesignOptionPair
                    {
                        ElementId = elementId,
                        DesignOptionId = designOptionId
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElementsToDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("a60bd42c-4359-4991-a78b-3b33ce7e0fbd");
    }
}
