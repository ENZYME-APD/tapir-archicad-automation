using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetCategoryValuesOfDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCategoryValuesOfDefaults";

        public GetCategoryValuesOfDefaultsComponent()
            : base(
                "GetCategoryValuesOfDefaults",
                "Gets the category values (Structural Function, Position, Renovation Status ...) of the tool " +
                "defaults of the given element types, the values new elements of the type get, optionally " +
                "only for the given category types. A category the default has no value in is listed without " +
                "categoryValueId; an element type without any of the requested categories gets an error " +
                "item. The names are localized: use the type and the categoryValueId in scripts.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "The element types whose tool defaults (the settings new elements get) are read.");

            InTexts(
                "Categories",
                CategoryValuesHelp.CategoriesDescription);

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            CategoryValuesHelp.AddOutputs(this);
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
                    "categoryValuesOfDefaults"));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCategoryValuesOfDefaults;

        public override Guid ComponentGuid =>
            new Guid("5bdfd4c4-75bf-43b4-88e3-b6ae2455d5d9");
    }
}
