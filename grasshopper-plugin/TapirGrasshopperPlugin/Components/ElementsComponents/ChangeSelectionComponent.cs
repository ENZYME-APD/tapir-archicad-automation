using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ChangeSelectionComponent : ArchicadAccessorComponent
    {
        public static string Doc => "Change Selection, add or remove elements.";
        public override string CommandName => "ChangeSelectionOfElements";

        public class ChangeSelectionParameters
        {
            [JsonProperty("addElementsToSelection")]
            public List<ElementIdItemObj> AddElementsToSelection;

            [JsonProperty("removeElementsFromSelection")]
            public List<ElementIdItemObj> RemoveElementsFromSelection;
        }

        public ChangeSelectionComponent()
            : base(
                "Change Selection",
                "ChangeSelection",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            AddGenerics(
                "ElementsToAdd",
                "Elements to add to selection.");

            AddGenerics(
                "ElementsToRemove",
                "Elements to remove from selection.");

            AddBoolean(
                "ClearSelection",
                "Remove all Elements from selection (before adding the given elements to selection).");

            Params.Input[0].Optional = true;
            Params.Input[1].Optional = true;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            ElementsObj elementsToAdd = ElementsObj.Create(
                da,
                0);
            ElementsObj elementsToRemove = ElementsObj.Create(
                da,
                1);
            bool clearSelection = false;
            if (!da.GetData(
                    2,
                    ref clearSelection))
            {
                return;
            }

            if ((elementsToAdd == null || elementsToAdd.Elements.Count == 0) &&
                (elementsToRemove == null ||
                 elementsToRemove.Elements.Count == 0) && !clearSelection)
            {
                return;
            }

            HashSet<ElementIdItemObj> uniqueElementsToRemove = new();
            if (elementsToRemove != null)
            {
                uniqueElementsToRemove.UnionWith(elementsToRemove.Elements);
            }

            if (clearSelection)
            {
                CommandResponse responseOfGetSelection =
                    SendArchicadAddOnCommand(
                        "GetSelectedElements",
                        null);
                if (responseOfGetSelection.Succeeded)
                {
                    ElementsObj selectedElements = responseOfGetSelection.Result
                        .ToObject<ElementsObj>();
                    uniqueElementsToRemove.UnionWith(selectedElements.Elements);
                }
            }

            if (elementsToAdd != null)
            {
                uniqueElementsToRemove.ExceptWith(elementsToAdd.Elements);
            }

            ChangeSelectionParameters parameters = new()
            {
                AddElementsToSelection =
                    elementsToAdd != null
                        ? elementsToAdd.Elements
                        : new List<ElementIdItemObj>(),
                RemoveElementsFromSelection =
                    uniqueElementsToRemove.ToList()
            };

            GetResponse(
                CommandName,
                parameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeSelection;

        public override Guid ComponentGuid =>
            new("d93b983a-35b3-436e-898f-87a79facbec5");
    }
}