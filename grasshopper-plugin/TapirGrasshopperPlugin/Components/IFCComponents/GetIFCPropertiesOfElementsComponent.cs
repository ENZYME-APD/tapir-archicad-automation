using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCPropertiesOfElementsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetIFCPropertiesOfElements";

        public GetIFCPropertiesOfElementsComponent()
            : base(
                "GetIFCPropertiesOfElements",
                "Retrieve the IFC properties of the given elements.",
                GroupNames.IFC)
        {
        }

        protected override string ResponseArrayKey => "elementIFCProperties";

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried element.");

            OutTextTree(
                "PropertySetNames",
                "Property set name of each IFC property (one branch per element).");

            OutTextTree(
                "PropertyNames",
                "Name of each IFC property (one branch per element).");

            OutTextTree(
                "PropertyValues",
                "Value of each IFC property (one branch per element).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var elementGuids = new List<object>();
            var propertySetNames = new DataTree<object>();
            var propertyNames = new DataTree<object>();
            var propertyValues = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                propertySetNames.EnsurePath(path);
                propertyNames.EnsurePath(path);
                propertyValues.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementIdOf(item));

                if (item["ifcProperties"] is JArray properties)
                {
                    foreach (var property in properties)
                    {
                        propertySetNames.Add(
                            JsonOutputHelp.Scalar(property, "propertySetName"),
                            path);
                        propertyNames.Add(
                            JsonOutputHelp.Scalar(property, "name"),
                            path);
                        propertyValues.Add(
                            JsonOutputHelp.Scalar(property, "value"),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementGuids);
            da.SetDataTree(1, propertySetNames);
            da.SetDataTree(2, propertyNames);
            da.SetDataTree(3, propertyValues);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCPropertiesOfElements;

        public override Guid ComponentGuid =>
            new Guid("5da0e87b-0ec0-44c9-8d66-c377ad46d437");
    }
}
