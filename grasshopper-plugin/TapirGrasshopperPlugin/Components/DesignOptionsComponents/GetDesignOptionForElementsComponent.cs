using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.DesignOptions;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.DesignOptionsComponents
{
    public class GetDesignOptionForElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetDesignOptionForElements";

        public GetDesignOptionForElementsComponent()
            : base(
                "GetDesignOptionForElements",
                "Get the design option association of the given elements. Available from Archicad 29.",
                GroupNames.DesignOptions)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifiers of the queried elements.");

            OutTexts(
                "Types",
                "Type of the design option association: NotExistingElement, MissingDesignOption, NotLinkedToAnyDesignOption or LinkedToDesignOption.");

            OutGenerics(
                "DesignOptionGuids",
                "Identifiers of the associated design options (empty when the element is not linked to a design option).");

            OutTexts(
                "Names",
                "Names of the associated design options.");

            OutTexts(
                "Ids",
                "String ids of the associated design options.");

            OutTexts(
                "OwnerSetNames",
                "Names of the owner design option sets.");
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
                    JHelp.Deserialize<GetDesignOptionForElementsResponse>,
                    out GetDesignOptionForElementsResponse response))
            {
                return;
            }

            var designOptionForElements =
                response.DesignOptionForElements ?? new List<DesignOptionForElement>();

            da.SetDataList(
                0,
                designOptionForElements.Select(
                    e => new ElementGuidWrapper { ElementId = e.ElementId }));

            da.SetDataList(
                1,
                designOptionForElements.Select(e => e.Type));

            da.SetDataList(
                2,
                designOptionForElements.Select(
                    e => e.DesignOption != null
                        ? new DesignOptionGuidWrapper { DesignOptionId = e.DesignOption.DesignOptionId }
                        : null));

            da.SetDataList(
                3,
                designOptionForElements.Select(e => e.DesignOption?.Name));

            da.SetDataList(
                4,
                designOptionForElements.Select(e => e.DesignOption?.Id));

            da.SetDataList(
                5,
                designOptionForElements.Select(e => e.DesignOption?.OwnerSetName));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDesignOptionForElements;

        public override Guid ComponentGuid =>
            new Guid("fdc2f54a-78ef-4c66-8bbf-5c9dff42ceaa");
    }
}
