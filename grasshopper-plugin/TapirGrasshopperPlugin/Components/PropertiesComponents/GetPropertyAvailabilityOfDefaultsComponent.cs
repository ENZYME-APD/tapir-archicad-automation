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
    public class GetPropertyAvailabilityOfDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetPropertyAvailabilityOfDefaults";

        public GetPropertyAvailabilityOfDefaultsComponent()
            : base(
                "GetPropertyAvailabilityOfDefaults",
                "Tell whether the given properties are available and whether they are visible in the current " +
                "tool default of element types, before creating new elements. It checks the native tool default " +
                "only, not a Favorite that is not applied. Unknown properties are reported as errors, not as " +
                "unavailable. Outputs have one branch per element type, with one item per property. " +
                "Available from Archicad 26.",
                GroupNames.Properties)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "PropertyGuids",
                "Identifiers of the properties to check.");

            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to check.");
        }

        protected override void AddOutputs()
        {
            OutBooleanTree(
                "Available",
                "Whether each property is available for the tool default (one branch per element type; null " +
                "when failed).");

            OutBooleanTree(
                "Visible",
                "Whether each property is visible for the tool default (one branch per element type; null when " +
                "failed).");

            OutTextTree(
                "PropertyErrorMessages",
                "Error message for each element type and property (one branch per element type; empty when " +
                "successful).");

            OutErrorMessages(
                "Error message for each element type (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                1);
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

            if (!da.TryGetList(
                    1,
                    out List<string> elementTypes))
            {
                return;
            }

            var available = new DataTree<object>();
            var visible = new DataTree<object>();
            var propertyErrors = new DataTree<string>();
            var errors = new List<string>();

            // The command checks the default of one element type per call.
            for (var i = 0; i < elementTypes.Count; i++)
            {
                var path = new GH_Path(i);
                available.EnsurePath(path);
                visible.EnsurePath(path);
                propertyErrors.EnsurePath(path);

                var parameters = new JObject
                {
                    ["elementType"] = elementTypes[i],
                    ["properties"] = JArray.FromObject(properties.Properties)
                };

                if (!TryGetCadResponse(
                        CommandName,
                        parameters,
                        ToAddOn,
                        out JObject response))
                {
                    errors.Add(
                        $"Failed to check the properties of the {elementTypes[i]} defaults.");
                    continue;
                }

                errors.Add("");

                foreach (var item in JsonOutputHelp.Items(
                             response,
                             "propertyAvailabilities"))
                {
                    if (JsonOutputHelp.IsError(item))
                    {
                        available.Add(null, path);
                        visible.Add(null, path);
                        propertyErrors.Add(
                            JsonOutputHelp.ErrorMessage(item),
                            path);
                        continue;
                    }

                    available.Add(JsonOutputHelp.Scalar(item, "available"), path);
                    visible.Add(JsonOutputHelp.Scalar(item, "visible"), path);
                    propertyErrors.Add("", path);
                }
            }

            da.SetDataTree(0, available);
            da.SetDataTree(1, visible);
            da.SetDataTree(2, propertyErrors);
            da.SetDataList(3, errors);
        }

        // Shares the icon of GetPropertyValues.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetPropertyValues;

        public override Guid ComponentGuid =>
            new Guid("8f751f70-e5cc-4c28-b9f3-bc603f14e351");
    }
}
