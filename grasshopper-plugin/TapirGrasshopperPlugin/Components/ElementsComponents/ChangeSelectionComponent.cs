using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ChangeSelectionComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "ChangeSelectionOfElements";

        public ChangeSelectionComponent()
            : base(
                "ChangeSelection",
                "Change Selection, add or remove elements.",
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics("ElementsToAdd");
            InGenerics("ElementsToRemove");
            InBoolean("ClearSelection");

            Params.Input[0].Optional = true;
            Params.Input[1].Optional = true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elementsToAdd))
            {
                return;
            }

            if (!da.TryCreateFromList(
                    1,
                    out ElementsObject elementsToRemove))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out bool clearSelection))
            {
                return;
            }

            if (elementsToAdd.Elements.Count == 0 &&
                elementsToRemove.Elements.Count == 0 && !clearSelection)
            {
                return;
            }

            var uniqueElementsToRemove = new HashSet<ElementGuidWrapper>();

            if (elementsToRemove != null)
            {
                uniqueElementsToRemove.UnionWith(elementsToRemove.Elements);
            }

            if (clearSelection)
            {
                var responseOfGetSelection = ToAddOn(
                    "GetSelectedElements",
                    null);
                if (responseOfGetSelection.Succeeded)
                {
                    var selectedElements = responseOfGetSelection.Result
                        .ToObject<ElementsObject>();
                    uniqueElementsToRemove.UnionWith(selectedElements.Elements);
                }
            }

            if (elementsToAdd != null)
            {
                uniqueElementsToRemove.ExceptWith(elementsToAdd.Elements);
            }

            var parameters = new ChangeSelectionParameters()
            {
                AddElementsToSelection =
                    elementsToAdd != null
                        ? elementsToAdd.Elements
                        : new List<ElementGuidWrapper>(),
                RemoveElementsFromSelection =
                    uniqueElementsToRemove.ToList()
            };


            SetValues(
                CommandName,
                parameters,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeSelection;

        public override Guid ComponentGuid =>
            new Guid("d93b983a-35b3-436e-898f-87a79facbec5");
    }
}