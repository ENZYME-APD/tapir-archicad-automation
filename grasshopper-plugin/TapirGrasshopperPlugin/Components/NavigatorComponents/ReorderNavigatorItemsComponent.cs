using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class ReorderNavigatorItemsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ReorderNavigatorItems";

        public ReorderNavigatorItemsComponent()
            : base(
                "ReorderNavigatorItems",
                "Reorders the children of a navigator item to match the given order. Works around " +
                "MoveNavigatorItem being unable to reposition an item within its current parent, by bouncing it " +
                "through a sibling.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ParentNavigatorItemId",
                "The folder whose children should be reordered.");

            InGenerics(
                "OrderedNavigatorItemIds",
                "The complete, desired order of every child of ParentNavigatorItemId. Every current child must be " +
                "listed.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each move done to reach the order (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid parentNavigatorItemId) ||
                !da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var orderedIds = new JArray();
            foreach (var wrapper in wrappers)
            {
                var id = GuidObject<NavigatorGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid navigator item identifier in the input OrderedNavigatorItemIds.");
                    return;
                }

                orderedIds.Add(new JObject { ["navigatorItemId"] = JObject.FromObject(id) });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject
                {
                    ["parentNavigatorItemId"] = JObject.FromObject(parentNavigatorItemId),
                    ["orderedNavigatorItemIds"] = orderedIds
                },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("b5f77da0-0d2c-485e-8058-8661067ced26");
    }
}
