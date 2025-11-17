using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class ChangeSelectionComponent : ArchicadAccessorComponent
    {
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
                "Change Selection, add or remove elements.",
                "Elements")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementsToAdd",
                "ElementsToAdd",
                "Elements to add to selection.",
                GH_ParamAccess.list);
            pManager.AddGenericParameter(
                "ElementsToRemove",
                "ElementsToRemove",
                "Elements to remove from selection.",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "ClearSelection",
                "ClearSelection",
                "Remove all Elements from selection (before adding the given elements to selection).",
                GH_ParamAccess.item,
                @default: false);

            Params.Input[0].Optional = true;
            Params.Input[1].Optional = true;
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var elementsToAdd = ElementsObj.Create(
                DA,
                0);
            var elementsToRemove = ElementsObj.Create(
                DA,
                1);
            var clearSelection = false;
            if (!DA.GetData(
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
            var parametersObj = JObject.FromObject(parameters);
            var response = SendArchicadAddOnCommand(
                "ChangeSelectionOfElements",
                parametersObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.ChangeSelection;

        public override Guid ComponentGuid =>
            new Guid("d93b983a-35b3-436e-898f-87a79facbec5");
    }
}