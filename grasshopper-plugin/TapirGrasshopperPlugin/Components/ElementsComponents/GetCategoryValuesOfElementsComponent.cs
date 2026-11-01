using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // The inputs and outputs GetCategoryValuesOfElements and
    // GetCategoryValuesOfDefaults share.
    public static class CategoryValuesHelp
    {
        public static string CategoriesDescription =>
            "Optional, the category types to read: StructuralFunction, Position, RenovationStatus, " +
            "RenovationFilter or BRI (German Archicad only). Left out: every category.";

        public static void AddOutputs(
            Component component)
        {
            component.OutTextTree(
                "CategoryTypes",
                "The type of each category (one branch per input item).");

            component.OutTextTree(
                "CategoryGuids",
                "The identifier of each category (one branch per input item).");

            component.OutTextTree(
                "CategoryValueGuids",
                "The identifier of the value in each category (one branch per input item; null when " +
                "there is no value).");

            component.OutTextTree(
                "CategoryValueNames",
                "The localized name of the value in each category, for display only (one branch per input " +
                "item).");

            component.OutErrorMessages(
                "Error message of each input item (empty when successful).");
        }

        // Adds the optional categories key from the input at the given index.
        public static void AddCategories(
            IGH_DataAccess da,
            int index,
            JObject parameters)
        {
            var categories = da.GetOptionals<string>(index);
            if (categories.Count > 0)
            {
                parameters["categories"] = JArray.FromObject(categories);
            }
        }

        public static void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var types = new DataTree<object>();
            var categoryGuids = new DataTree<object>();
            var valueGuids = new DataTree<object>();
            var valueNames = new DataTree<object>();
            var errors = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                types.EnsurePath(path);
                categoryGuids.EnsurePath(path);
                valueGuids.EnsurePath(path);
                valueNames.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["categoryValues"] is JArray values))
                {
                    continue;
                }

                foreach (var value in values)
                {
                    types.Add(
                        JsonOutputHelp.Scalar(
                            value,
                            "type"),
                        path);
                    categoryGuids.Add(
                        JsonOutputHelp.GuidOf(value["categoryId"]),
                        path);
                    valueGuids.Add(
                        JsonOutputHelp.GuidOf(value["categoryValueId"]),
                        path);
                    valueNames.Add(
                        JsonOutputHelp.Scalar(
                            value,
                            "name"),
                        path);
                }
            }

            da.SetDataTree(
                0,
                types);
            da.SetDataTree(
                1,
                categoryGuids);
            da.SetDataTree(
                2,
                valueGuids);
            da.SetDataTree(
                3,
                valueNames);
            da.SetDataList(
                4,
                errors);
        }
    }

    public class GetCategoryValuesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCategoryValuesOfElements";

        public GetCategoryValuesOfElementsComponent()
            : base(
                "GetCategoryValuesOfElements",
                "Gets the category values (Structural Function, Position, Renovation Status ...) of the given " +
                "elements, optionally only for the given category types. An element that has none of the " +
                "requested categories (an annotation, for example) gets an error item. The names are " +
                "localized: use the type and the categoryValueId in scripts.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to get the category values of.");

            InTexts(
                "Categories",
                CategoryValuesHelp.CategoriesDescription);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            CategoryValuesHelp.AddOutputs(this);
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
            CategoryValuesHelp.AddCategories(
                da,
                1,
                parameters);

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            CategoryValuesHelp.SetOutputs(
                da,
                JsonOutputHelp.Items(
                    response,
                    "categoryValuesOfElements"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCategoryValuesOfElements;

        public override Guid ComponentGuid =>
            new Guid("26cf593a-1914-4503-898f-e606c93eebfe");
    }
}
