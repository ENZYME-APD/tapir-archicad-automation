using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class SetPropertyValuesOfElementsComponent : Component
    {
        public SetPropertyValuesOfElementsComponent ()
          : base (
                "Set Property Values",
                "SetPropertyValues",
                "Set property values of elements.",
                "Properties"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("PropertyId", "PropertyId", "The property id to set the value for.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to set the value for.", GH_ParamAccess.list);
            pManager.AddTextParameter ("Values", "Values", "Single value or list of values to set for the corresponding elements.", GH_ParamAccess.list);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {

        }

        protected override void SolveInstance (IGH_DataAccess DA)
        {
            PropertyIdObj propertyId = new PropertyIdObj ();
            if (!DA.GetData (0, ref propertyId)) {
                return;
            }
            List<ElementIdItemObj> elements = new List<ElementIdItemObj> ();
            if (!DA.GetDataList (1, elements)) {
                return;
            }
            List<string> values = new List<string> ();
            if (!DA.GetDataList (2, values)) {
                return;
            }
            if (values.Count != 1 && elements.Count != values.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The count of Values must be 1 or the same as the count of ElementIds.");
                return;
            }

            ElementPropertyValuesObj elemPropertyValues = new ElementPropertyValuesObj ();
            elemPropertyValues.ElementPropertyValues = new List<ElementPropertyValueObj> ();

            for (int i = 0; i < elements.Count; i++) {
                ElementIdItemObj elementId = elements[i];
                ElementPropertyValueObj elemPropertyValue = new ElementPropertyValueObj () {
                    ElementId = elementId.ElementId,
                    PropertyId = propertyId,
                    PropertyValue = new PropertyValueObj () {
                        Value = values.Count == 1 ? values[0] : values[i]
                    }
                };
                elemPropertyValues.ElementPropertyValues.Add (elemPropertyValue);
            }

            JObject elemPropertyValuesObj = JObject.FromObject (elemPropertyValues);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "SetPropertyValuesOfElements", elemPropertyValuesObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => Properties.Resources.SetPropertyValues;

        public override Guid ComponentGuid => new Guid ("5d2aa76e-4a59-4b58-a5ce-51878c1478d0");
    }
}
