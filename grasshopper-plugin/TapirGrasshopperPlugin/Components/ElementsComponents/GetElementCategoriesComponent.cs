using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementCategoriesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementCategories";

        public GetElementCategoriesComponent()
            : base(
                "GetElementCategories",
                "Lists the element categories of the project (Structural Function, Position, Renovation " +
                "Status, Renovation Filter and, in the German version, BRI) with their possible values. The " +
                "names of the categories and values are localized: identify a category by its type and a " +
                "value by its categoryValueId, never by name.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "CategoryTypes",
                "The type of each category: StructuralFunction, Position, RenovationStatus, " +
                "RenovationFilter or BRI.");

            OutTexts(
                "CategoryGuids",
                "The identifier of each category.");

            OutTexts(
                "CategoryNames",
                "The localized name of each category, for display only.");

            OutTextTree(
                "CategoryValueGuids",
                "The identifiers of the possible values of each category (one branch per category).");

            OutTextTree(
                "CategoryValueNames",
                "The localized names of the possible values, for display only (one branch per category).");

            OutBooleanTree(
                "IsVisibleOnUI",
                "Whether each value is offered in the settings dialogs of Archicad (one branch per category).");

            OutBooleanTree(
                "UsableAsCriteria",
                "Whether each value can be used as a criterion, for example in Find & Select and in " +
                "Graphic Override rules (one branch per category).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var types = new List<object>();
            var guids = new List<object>();
            var names = new List<object>();
            var valueGuids = new DataTree<object>();
            var valueNames = new DataTree<object>();
            var isVisibleOnUI = new DataTree<object>();
            var usableAsCriteria = new DataTree<object>();

            var categories = JsonOutputHelp.Items(
                response,
                "categories");
            for (var i = 0; i < categories.Count; i++)
            {
                var category = categories[i];
                var path = new GH_Path(i);
                valueGuids.EnsurePath(path);
                valueNames.EnsurePath(path);
                isVisibleOnUI.EnsurePath(path);
                usableAsCriteria.EnsurePath(path);

                types.Add(
                    JsonOutputHelp.Scalar(
                        category,
                        "type"));
                guids.Add(JsonOutputHelp.GuidOf(category["categoryId"]));
                names.Add(
                    JsonOutputHelp.Scalar(
                        category,
                        "name"));

                if (!(category["values"] is JArray values))
                {
                    continue;
                }

                foreach (var value in values)
                {
                    valueGuids.Add(
                        JsonOutputHelp.GuidOf(value["categoryValueId"]),
                        path);
                    valueNames.Add(
                        JsonOutputHelp.Scalar(
                            value,
                            "name"),
                        path);
                    isVisibleOnUI.Add(
                        JsonOutputHelp.Scalar(
                            value,
                            "isVisibleOnUI"),
                        path);
                    usableAsCriteria.Add(
                        JsonOutputHelp.Scalar(
                            value,
                            "usableAsCriteria"),
                        path);
                }
            }

            da.SetDataList(
                0,
                types);
            da.SetDataList(
                1,
                guids);
            da.SetDataList(
                2,
                names);
            da.SetDataTree(
                3,
                valueGuids);
            da.SetDataTree(
                4,
                valueNames);
            da.SetDataTree(
                5,
                isVisibleOnUI);
            da.SetDataTree(
                6,
                usableAsCriteria);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementCategories;

        public override Guid ComponentGuid =>
            new Guid("2ab10ca0-5b4a-4119-b61a-d34d4a80863c");
    }
}
