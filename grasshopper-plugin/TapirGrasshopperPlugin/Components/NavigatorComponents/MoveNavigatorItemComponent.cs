using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class MoveNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "MoveNavigatorItem";

        public MoveNavigatorItem()
            : base(
                "MoveNavigatorItem",
                "Moves a navigator item to a new parent in the navigator tree, or to a new position within its " +
                "current parent. Fails when Archicad does not put the item where it was asked to.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemIds",
                "Identifier of the navigator item to move.");

            InGeneric(
                "ParentNavigatorItemId",
                "The new parent of the item. It can be the item's current parent, to change the order of the " +
                "items in it.");

            InGeneric(
                "PreviousNavigatorItemId",
                "Optional child of ParentNavigatorItemId that the item is placed right after. Without it an " +
                "item moved within its own parent goes to the end of it.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid navigatorItemIdToMove))
            {
                return;
            }

            if (!da.TryCreate(
                    1,
                    out NavigatorGuid parentNavigatorItemId))
            {
                return;
            }

            if (!da.TryCreate(
                    2,
                    out NavigatorGuid previousNavigatorItemId))
            {
                previousNavigatorItemId = null;
            }

            var parameters = new JObject
            {
                ["navigatorItemIdToMove"] = JObject.FromObject(navigatorItemIdToMove),
                ["parentNavigatorItemId"] = JObject.FromObject(parentNavigatorItemId)
            };

            if (previousNavigatorItemId != null)
            {
                parameters["previousNavigatorItemId"] =
                    JObject.FromObject(previousNavigatorItemId);
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(
                0,
                response.Message());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("fef582f6-07bd-4ffd-92cd-e41cb0447e2d");
    }
}