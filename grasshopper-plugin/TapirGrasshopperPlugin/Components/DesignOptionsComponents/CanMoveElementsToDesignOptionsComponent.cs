using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class CanMoveElementsToDesignOptionsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "CanMoveElementsToDesignOptions";

        public CanMoveElementsToDesignOptionsComponent()
            : base(
                "CanMoveElementsToDesignOptions",
                "Checks whether the given elements can be moved to another design option or to the main model, with the reason when they cannot. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to check.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the checked elements.");

            OutBooleans(
                "CanMove",
                "Whether each element can be moved to another design option or to the main model.");

            OutTexts(
                "Reasons",
                "The reason why each element cannot be moved (empty when it can).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    elements,
                    ToAddOn,
                    JHelp.Deserialize<CanMoveElementsToDesignOptionsResponse>,
                    out CanMoveElementsToDesignOptionsResponse response))
            {
                return;
            }

            var canMoveElements =
                response.CanMoveElements ?? new List<CanMoveElement>();

            da.SetDataList(
                0,
                canMoveElements.Select(
                    e => new ElementGuidWrapper { ElementId = e.ElementId }));

            da.SetDataList(
                1,
                canMoveElements.Select(e => e.CanMove));

            da.SetDataList(
                2,
                canMoveElements.Select(e => e.Error?.Message));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElementsToDesignOptions;

        public override Guid ComponentGuid =>
            new Guid("3c9e7b52-8f41-4d2a-b6e0-91a5d7c4f2e8");
    }
}
