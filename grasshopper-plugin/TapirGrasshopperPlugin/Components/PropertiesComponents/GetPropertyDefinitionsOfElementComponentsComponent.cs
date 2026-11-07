using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertyDefinitionsOfElementComponentsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyDefinitionsOfElementComponents";

        public GetPropertyDefinitionsOfElementComponentsComponent()
            : base(
                "GetPropertyDefinitionsOfElementComponents",
                "Get the property definitions available on the given element components (from " +
                "GetComponentsOfElements): the user defined, the built-in, or all of them. " +
                "The outputs have one branch per queried component.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                ElementComponentHelp.ElementGuidsDescription);

            InTexts(
                "ComponentGuids",
                ElementComponentHelp.ComponentGuidsDescription);

            InText(
                "Filter",
                "Which property definitions to return: UserDefined, BuiltIn or All (the default).");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "PropertyGuids",
                "Identifiers of the property definitions of each component (one branch per component).");

            OutTextTree(
                "PropertyNames",
                "Name of each property definition, in the structure of PropertyGuids.");

            OutTextTree(
                "PropertyTypes",
                "Type of each property definition (StaticBuiltIn, DynamicBuiltIn or Custom), in the " +
                "structure of PropertyGuids.");

            OutErrorMessages(
                "Error message for each queried component (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ElementComponentHelp.TryGetElementComponents(
                    this,
                    da,
                    0,
                    1,
                    out JArray elementComponents))
            {
                return;
            }

            var parameters = new JObject { ["elementComponents"] = elementComponents };

            if (da.TryGet(
                    2,
                    out string filter) &&
                !string.IsNullOrEmpty(filter))
            {
                parameters["filter"] = filter.Trim();
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var propertyGuids = new DataTree<object>();
            var propertyNames = new DataTree<string>();
            var propertyTypes = new DataTree<string>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "propertyDefinitionsOfElementComponents");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                propertyGuids.EnsurePath(path);
                propertyNames.EnsurePath(path);
                propertyTypes.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                foreach (var definition in JsonOutputHelp.Items(
                             item as JObject,
                             "propertyDefinitions"))
                {
                    propertyGuids.Add(
                        PropertyGuidObject.FromString(
                            JsonOutputHelp.WrappedGuidOf(
                                definition,
                                "propertyId")),
                        path);
                    propertyNames.Add(
                        definition?["propertyName"]?.ToString(),
                        path);
                    propertyTypes.Add(
                        definition?["propertyType"]?.ToString(),
                        path);
                }
            }

            da.SetDataTree(0, propertyGuids);
            da.SetDataTree(1, propertyNames);
            da.SetDataTree(2, propertyTypes);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyDefinitionsOfElementComponents;

        public override Guid ComponentGuid =>
            new Guid("3179fddd-8090-45ef-b61a-151d49b2fd27");
    }
}
