using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Properties;

namespace TapirGrasshopperPlugin.Components.PropertiesComponents
{
    public class GetPropertiesOfToolDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertiesOfToolDefaults";

        public GetPropertiesOfToolDefaultsComponent()
            : base(
                "GetPropertiesOfToolDefaults",
                "Get the property definitions of the tool defaults (the settings new elements get) of the given " +
                "element types, filtered by kind (all by default), and optionally the value of each property in " +
                "the tool default. Outputs have one branch per queried element type.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to query.");

            InText(
                "Filter",
                "Which property definitions to return: UserDefined, FundamentalBuiltIn, UserLevelBuiltIn or " +
                "All (the default). From Archicad 28 FundamentalBuiltIn and UserLevelBuiltIn both return every " +
                "built-in property.");

            InBoolean(
                "IncludeValues",
                "Also get the value of each property in the tool default. Default is false.");

            SetOptionality(
                new[]
                {
                    1,
                    2
                });
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "PropertyGuids",
                "Identifier of each property of the tool default (one branch per element type).");

            OutTextTree(
                "PropertyNames",
                "Name of each property (one branch per element type).");

            OutGenericTree(
                "PropertyGroupGuids",
                "Identifier of the group of each property (one branch per element type).");

            OutTextTree(
                "Values",
                "Value of each property in the tool default (one branch per element type; " +
                "null when it has none). Only filled when IncludeValues is true.");

            OutTextTree(
                "ValueErrorMessages",
                "Why the value of each property could not be read (one branch per element type; " +
                "empty when successful). Only filled when IncludeValues is true.");

            OutErrorMessages(
                "Error message for each queried element type (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes))
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes)
            };

            if (da.TryGet(
                    1,
                    out string filter) &&
                !string.IsNullOrEmpty(filter))
            {
                parameters["filter"] = filter;
            }

            da.TryGet(
                2,
                out bool includeValues);
            if (includeValues)
            {
                parameters["includeValues"] = true;
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
            var propertyNames = new DataTree<object>();
            var propertyGroupGuids = new DataTree<object>();
            var values = new DataTree<object>();
            var valueErrors = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "propertiesOfToolDefaults");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                propertyGuids.EnsurePath(path);
                propertyNames.EnsurePath(path);
                propertyGroupGuids.EnsurePath(path);
                values.EnsurePath(path);
                valueErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (!(item["properties"] is JArray properties))
                {
                    continue;
                }

                foreach (var property in properties)
                {
                    propertyGuids.Add(
                        PropertyGuidObject.FromString(
                            JsonOutputHelp.WrappedGuidOf(
                                property,
                                "propertyId")),
                        path);
                    propertyNames.Add(
                        property["propertyName"]?.ToString(),
                        path);
                    propertyGroupGuids.Add(
                        PropertyGroupGuidObject.FromString(
                            JsonOutputHelp.WrappedGuidOf(
                                property,
                                "propertyGroupId")),
                        path);

                    if (includeValues)
                    {
                        values.Add(
                            JsonOutputHelp.Scalar(
                                property["propertyValue"],
                                "value"),
                            path);
                        valueErrors.Add(
                            JsonOutputHelp.ErrorMessage(property),
                            path);
                    }
                }
            }

            da.SetDataTree(0, propertyGuids);
            da.SetDataTree(1, propertyNames);
            da.SetDataTree(2, propertyGroupGuids);
            da.SetDataTree(3, values);
            da.SetDataTree(4, valueErrors);
            da.SetDataList(5, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertiesOfToolDefaults;

        public override Guid ComponentGuid =>
            new Guid("9b174be1-d4a5-4225-8795-a3f8e826c6ce");
    }
}
