using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class SetCategoryValuesOfElementsComponent
        : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCategoryValuesOfElements";

        public SetCategoryValuesOfElementsComponent()
            : base(
                "SetCategoryValuesOfElements",
                "Sets the category values (Structural Function, Position, Renovation Status ...) of the given " +
                "elements, one element and one category per item, in a single undoable step. The value must " +
                "belong to the given category, as GetElementCategories lists them; an element without that " +
                "category gets a failed execution result. The names are localized: use the type and the " +
                "categoryValueId in scripts.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "The elements to set a category value of. The same element can appear several times, once " +
                "per category.");

            InTexts(
                "CategoryTypes",
                "The category type of each item: StructuralFunction, Position, RenovationStatus, " +
                "RenovationFilter or BRI (input only 1 to use the same value for all).");

            InTexts(
                "CategoryValueGuids",
                "A value of the category of each item, as GetElementCategories lists them (input only 1 " +
                "to use the same value for all).");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each item (empty when the value was set).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !da.TryGetList(
                    1,
                    out List<string> types) ||
                !da.TryGetList(
                    2,
                    out List<string> valueGuids))
            {
                return;
            }

            var elementIds = ElementInputHelp.ElementGuids(
                this,
                elementWrappers,
                "ElementGuids");
            if (elementIds == null ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    types,
                    elementIds.Count,
                    "CategoryTypes") ||
                !ElementInputHelp.CheckOneOrMany(
                    this,
                    valueGuids,
                    elementIds.Count,
                    "CategoryValueGuids"))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                items.Add(
                    new JObject
                    {
                        ["elementId"] = JObject.FromObject(elementIds[i]),
                        ["type"] = ElementInputHelp.ItemAt(
                            types,
                            i),
                        ["categoryValueId"] = new JObject
                        {
                            ["guid"] = ElementInputHelp.ItemAt(
                                valueGuids,
                                i)
                        }
                    });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementCategoryValues"] = items },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetCategoryValuesOfElements;

        public override Guid ComponentGuid =>
            new Guid("51a994c6-496e-4378-8e7b-9d4b9109caf1");
    }
}
