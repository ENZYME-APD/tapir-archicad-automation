using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyEditabilityComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetPropertyEditability";

        public GetPropertyEditabilityComponent()
            : base(
                "GetPropertyEditability",
                "Tell whether the values of the given properties can be modified, and whether the properties are " +
                "visible, for the given elements. Also tells whether they can be modified in the tool defaults of " +
                "element types (from Archicad 26) and in the default values of the property definitions " +
                "themselves. Unknown elements, element types and properties are reported as errors, not as " +
                "locked properties.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties to check.");

            InGenerics(
                "ElementGuids",
                "Elements to check the properties for: whether each value can be modified and whether each " +
                "property is visible.");

            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to check the properties for (from Archicad 26).");

            InBoolean(
                "PropertyDefinitionDefaults",
                "Also check whether the default value of each property definition can be modified (the default " +
                "of the definition itself, not the tool default of an element type). Default is false.");

            SetOptionality(new[] { 1, 2, 3 });
        }

        protected override void AddOutputs()
        {
            OutBooleanTree(
                "ElementEditable",
                "Whether each property value can be modified for each element (one branch per element, one item " +
                "per property; null when failed).");

            OutBooleanTree(
                "ElementVisible",
                "Whether each property is visible for each element (in the structure of ElementEditable).");

            OutTextTree(
                "ElementErrorMessages",
                "Error message for each element and property (in the structure of ElementEditable; empty when " +
                "successful).");

            OutBooleanTree(
                "ElementTypeDefaultEditable",
                "Whether each property value can be modified in the tool default of each element type (one " +
                "branch per element type, one item per property; null when failed).");

            OutTextTree(
                "ElementTypeDefaultErrorMessages",
                "Error message for each element type and property (in the structure of " +
                "ElementTypeDefaultEditable; empty when successful).");

            OutBooleans(
                "PropertyDefinitionDefaultEditable",
                "Whether the default value of each property definition can be modified (only filled when " +
                "PropertyDefinitionDefaults is true; null when failed).");

            OutTexts(
                "PropertyDefinitionDefaultErrorMessages",
                "Error message for each property definition default (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out PropertiesObject properties))
            {
                return;
            }

            var elementWrappers = da.OptionalList<GH_ObjectWrapper>(1);
            var elementTypes = da.OptionalList<string>(2);
            var propertyDefinitionDefaults = da.GetOptional(
                3,
                false);

            if (!this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out List<JObject> elementIds))
            {
                return;
            }

            var parameters = new JObject
            {
                ["properties"] = JArray.FromObject(properties.Properties)
            };

            if (elementIds.Count > 0)
            {
                var elements = new JArray();
                foreach (var elementId in elementIds)
                {
                    elements.Add(new JObject { ["elementId"] = elementId });
                }
                parameters["elements"] = elements;
            }

            if (elementTypes.Count > 0)
            {
                parameters["elementTypes"] = new JArray(elementTypes);
            }

            if (propertyDefinitionDefaults)
            {
                parameters["propertyDefinitionDefaults"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var propertyCount = properties.Properties.Count;

            var elementEditable = new DataTree<object>();
            var elementVisible = new DataTree<object>();
            var elementErrors = new DataTree<string>();
            var elementItems = JsonOutputHelp.Items(
                response,
                "editabilityOfElements");
            for (var i = 0; i < elementItems.Count; i++)
            {
                var path = new GH_Path(i);
                elementEditable.EnsurePath(path);
                elementVisible.EnsurePath(path);
                elementErrors.EnsurePath(path);

                AddEditabilities(
                    elementItems[i],
                    propertyCount,
                    path,
                    elementEditable,
                    elementVisible,
                    elementErrors);
            }

            var typeEditable = new DataTree<object>();
            var typeErrors = new DataTree<string>();
            var typeItems = JsonOutputHelp.Items(
                response,
                "editabilityOfElementTypeDefaults");
            for (var i = 0; i < typeItems.Count; i++)
            {
                var path = new GH_Path(i);
                typeEditable.EnsurePath(path);
                typeErrors.EnsurePath(path);

                AddEditabilities(
                    typeItems[i],
                    propertyCount,
                    path,
                    typeEditable,
                    null,
                    typeErrors);
            }

            var definitionEditable = new List<object>();
            var definitionErrors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "editabilityOfPropertyDefinitionDefaults"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    definitionEditable.Add(null);
                    definitionErrors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                definitionEditable.Add(JsonOutputHelp.Scalar(item, "editable"));
                definitionErrors.Add("");
            }

            da.SetDataTree(0, elementEditable);
            da.SetDataTree(1, elementVisible);
            da.SetDataTree(2, elementErrors);
            da.SetDataTree(3, typeEditable);
            da.SetDataTree(4, typeErrors);
            da.SetDataList(5, definitionEditable);
            da.SetDataList(6, definitionErrors);
        }

        // One branch of an editabilityOfElements or
        // editabilityOfElementTypeDefaults item: one entry per property. An
        // error of the whole element (or element type) is repeated for each
        // property, so the branches keep the length of the property list.
        private static void AddEditabilities(
            JToken item,
            int propertyCount,
            GH_Path path,
            DataTree<object> editable,
            DataTree<object> visible,
            DataTree<string> errors)
        {
            if (JsonOutputHelp.IsError(item))
            {
                var message = JsonOutputHelp.ErrorMessage(item);
                for (var p = 0; p < propertyCount; p++)
                {
                    editable.Add(null, path);
                    visible?.Add(null, path);
                    errors.Add(message, path);
                }
                return;
            }

            foreach (var editability in JsonOutputHelp.Items(
                         item as JObject,
                         "propertyEditabilities"))
            {
                if (JsonOutputHelp.IsError(editability))
                {
                    editable.Add(null, path);
                    visible?.Add(null, path);
                    errors.Add(JsonOutputHelp.ErrorMessage(editability), path);
                    continue;
                }

                editable.Add(JsonOutputHelp.Scalar(editability, "editable"), path);
                visible?.Add(JsonOutputHelp.Scalar(editability, "visible"), path);
                errors.Add("", path);
            }
        }

        // Shares the icon of GetPropertyValues.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("ab27c6d5-2220-4769-936b-838125f549a5");
    }
}
