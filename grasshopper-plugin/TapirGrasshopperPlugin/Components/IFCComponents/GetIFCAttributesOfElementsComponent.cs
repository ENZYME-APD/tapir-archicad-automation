using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCAttributesOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetIFCAttributesOfElements";

        public GetIFCAttributesOfElementsComponent()
            : base(
                "GetIFCAttributesOfElements",
                "Retrieve the IFC attributes (Name, Description, ObjectType, Tag, PredefinedType, LongName, ...) " +
                "of the given elements, subelements included: the name, the IFC value type and the value of each " +
                "attribute. On Archicad 25, 26 and 27 StoredOnly returns only the attributes stored on the elements.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");

            InBoolean(
                "StoredOnly",
                "Return only the attributes stored on the elements, not the ones Archicad would generate on " +
                "IFC export. Used only in Archicad 25, 26 and 27. Default is false.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried element.");

            OutTextTree(
                "AttributeNames",
                "Name of each IFC attribute (one branch per element).");

            OutTextTree(
                "ValueTypes",
                "IFC type of the value of each attribute, for example IfcLabel, IfcText or IfcIdentifier " +
                "(one branch per element).");

            OutTextTree(
                "Values",
                "Value of each IFC attribute (one branch per element; null when the attribute has no value).");

            OutErrorMessages(
                "Error message for each queried element (empty when successful).");
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

            var parameters = JObject.FromObject(elements);

            da.TryGet(
                1,
                out bool storedOnly);
            if (storedOnly)
            {
                parameters["storedOnly"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new List<object>();
            var attributeNames = new DataTree<object>();
            var valueTypes = new DataTree<object>();
            var values = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementIFCAttributes");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                attributeNames.EnsurePath(path);
                valueTypes.EnsurePath(path);
                values.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(item));

                if (item["ifcAttributes"] is JArray attributes)
                {
                    foreach (var attribute in attributes)
                    {
                        attributeNames.Add(
                            JsonOutputHelp.Scalar(attribute, "name"),
                            path);
                        valueTypes.Add(
                            JsonOutputHelp.Scalar(attribute, "valueType"),
                            path);
                        values.Add(
                            JsonOutputHelp.Scalar(attribute, "value"),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementGuids);
            da.SetDataTree(1, attributeNames);
            da.SetDataTree(2, valueTypes);
            da.SetDataTree(3, values);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCAttributesOfElements;

        public override Guid ComponentGuid =>
            new Guid("81599455-421e-4dc7-a5d6-8dad80ef3a65");
    }
}
