using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyValuesOfElementsComponent : ArchicadAccessorComponent
    {
        public GetPropertyValuesOfElementsComponent ()
          : base (
                "Get Property Values",
                "GetPropertyValues",
                "Get property values of elements.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "The property id to get the value for.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to get the value for.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids of the found elements having the given property.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Values", "Values", "The property values of the elements.", GH_ParamAccess.list);
        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            PropertyIdObj propertyId = PropertyIdObj.Create (DA, 0);
            if (propertyId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input PropertyId failed to collect data.");
                return;
            }
            ElementsObj elements = ElementsObj.Create (DA, 1);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            ElementsAndPropertyIdsObj elementsAndPropertyIds = new ElementsAndPropertyIdsObj () {
                Elements = elements.Elements,
                PropertyIds = new List<PropertyIdItemObj> ()
            };
            elementsAndPropertyIds.PropertyIds.Add (new PropertyIdItemObj () {
                PropertyId = propertyId
            });

            JObject elementsAndPropertyIdsObj = JObject.FromObject (elementsAndPropertyIds);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "GetPropertyValuesOfElements", elementsAndPropertyIdsObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            PropertyValuesForElements propertyValuesForElements = response.Result.ToObject<PropertyValuesForElements> ();
            List<ElementIdItemObj> elementIds = new List<ElementIdItemObj> ();
            List<string> values = new List<string> ();
            for (int i = 0; i < propertyValuesForElements.PropertyValuesOrErrors.Count; i++) {
                PropertyValues propertyValuesOrError = propertyValuesForElements.PropertyValuesOrErrors[i];
                if (propertyValuesOrError.PropertyValuesOrErrors == null) {
                    continue;
                }

                foreach (PropertyValueOrError propertyValueOrError in propertyValuesOrError.PropertyValuesOrErrors) {
                    if (propertyValueOrError.PropertyValue == null) {
                        continue;
                    }

                    elementIds.Add (elements.Elements[i]);
                    values.Add (propertyValueOrError.PropertyValue.Value);
                }
            }

            DA.SetDataList (0, elementIds);
            DA.SetDataList (1, values);
        }

        // TODO
        // protected override System.Drawing.Bitmap Icon => Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid => new Guid ("c2a0a175-5cbc-4ec3-b5e2-744cd1be280d");
    }
}
