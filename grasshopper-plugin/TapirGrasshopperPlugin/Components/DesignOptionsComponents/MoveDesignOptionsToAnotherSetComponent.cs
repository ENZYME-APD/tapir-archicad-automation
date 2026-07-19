using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class MoveDesignOptionsToAnotherSetComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveDesignOptionsToAnotherSet";

        public MoveDesignOptionsToAnotherSetComponent()
            : base(
                "MoveDesignOptionsToAnotherSet",
                "Move the given design options to another sets. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DesignOptionGuids",
                "Identifiers of the design options to move.");

            InTexts(
                "SetNames",
                "Names of the target design option sets (input only 1 to move all design options into the same set).");
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

            if (!da.TryGetList(
                    1,
                    out List<string> setNames))
            {
                return;
            }

            if (setNames.Count != 1 &&
                setNames.Count != designOptionWrappers.Count)
            {
                this.AddError(
                    "The size of the input SetNames must be 1 or equal to the size of the input DesignOptionGuids.");
                return;
            }

            var input = new MoveDesignOptionsToAnotherSetParameters
            {
                DesignOptionAndSetPairs = new List<DesignOptionAndSetPair>()
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

                input.DesignOptionAndSetPairs.Add(
                    new DesignOptionAndSetPair
                    {
                        DesignOptionId = designOptionId,
                        SetName = setNames[setNames.Count == 1 ? 0 : i]
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveDesignOptionsToAnotherSet;

        public override Guid ComponentGuid =>
            new Guid("25c4e846-f00e-44f0-b848-3a8294ffe284");
    }
}
