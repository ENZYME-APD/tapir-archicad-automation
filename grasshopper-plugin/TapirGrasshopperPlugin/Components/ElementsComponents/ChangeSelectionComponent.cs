using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
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
            InGenerics(
                "ElementsToAdd",
                "Elements to add to selection.");

            InGenerics(
                "ElementsToRemove",
                "Elements to remove from selection.");

            InBoolean(
                "ClearSelection",
                "Remove all Elements from the selection (before adding the given elements to selection).");

            Params.Input[0].Optional = true;
            Params.Input[1].Optional = true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj elementsToAdd))
            {
                return;
            }

            if (!ElementsObj.TryCreate(
                    da,
                    1,
                    out ElementsObj elementsToRemove))
            {
                return;
            }

            if (!da.TryGetItem(
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

            var uniqueElementsToRemove = new HashSet<ElementIdItemObj>();

            if (elementsToRemove != null)
            {
                uniqueElementsToRemove.UnionWith(elementsToRemove.Elements);
            }

            if (clearSelection)
            {
                var responseOfGetSelection = SendArchicadAddOnCommand(
                    "GetSelectedElements",
                    null);
                if (responseOfGetSelection.Succeeded)
                {
                    var selectedElements = responseOfGetSelection.Result
                        .ToObject<ElementsObj>();
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
                        : new List<ElementIdItemObj>(),
                RemoveElementsFromSelection =
                    uniqueElementsToRemove.ToList()
            };

            SetArchiCadValues(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeSelection;

        public override Guid ComponentGuid =>
            new Guid("d93b983a-35b3-436e-898f-87a79facbec5");
    }
}