using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    // The Tapir command of the navigator item tree. Besides what the NavigatorTree
    // component (Archicad's own command) gives, it reports the IDs shown on the
    // navigator, whether the name and ID are custom, whether a view is
    // independent and, on request, the number of children of each item.
    public class GetNavigatorItemTreeComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetNavigatorItemTree";

        public GetNavigatorItemTreeComponent()
            : base(
                "GetNavigatorItemTree",
                "Returns the full navigator item tree for the specified map. With IncludeChildCount true every " +
                "item also reports the number of its direct children. The items are given in a tree: the " +
                "children of the root are in branch {0}, the children of its i-th child in branch {0;i}, and so on.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "NavigatorMapId",
                "The navigator map to retrieve: PublicViewMap, ProjectMap, LayoutBook or PublisherSets.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "PublisherSetName",
                "The name of the publisher set to retrieve. Used only when NavigatorMapId is PublisherSets. " +
                "Optional, without it the first publisher set is returned.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "IncludeChildCount",
                "Set to true to get ChildCounts, the number of direct children Archicad reports for every " +
                "item. Optional, defaults to false.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "NavigatorItemIds",
                "The identifier of each navigator item.");

            OutTextTree(
                "Types",
                "The type of each navigator item.");

            OutTextTree(
                "Names",
                "The name of each navigator item.");

            OutTextTree(
                "Prefixes",
                "The floor number for story items, an empty string for every other item type.");

            OutTextTree(
                "UiIds",
                "The identifier shown next to the name on the navigator.");

            OutBooleanTree(
                "CustomUiIds",
                "True when the identifier was typed by hand instead of being inherited from the Project Map source.");

            OutBooleanTree(
                "CustomNames",
                "True when the name was typed by hand instead of being inherited from the Project Map source.");

            OutBooleanTree(
                "IsIndependent",
                "True when the item is independent, that is when its link to the Project Map is broken.");

            OutGenericTree(
                "SourceNavigatorItemIds",
                "The identifier of the navigator item each one was created from (null when it has no source).");

            OutIntegerTree(
                "ChildCounts",
                "The number of direct children Archicad reports for each item (only with IncludeChildCount true).");

            OutGeneric(
                "RootNavigatorItemId",
                "The identifier of the root item of the map.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string navigatorMapId))
            {
                return;
            }

            var parameters = new JObject { ["navigatorMapId"] = navigatorMapId };

            NavigatorComponentHelp.CopyOptional<string>(
                da,
                1,
                parameters,
                "publisherSetName");

            NavigatorComponentHelp.CopyOptional<bool>(
                da,
                2,
                parameters,
                "includeChildCount");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var trees = new Trees();
            var root = response["navigatorItemTree"];

            AddChildren(
                root,
                new GH_Path(0),
                trees);

            da.SetDataTree(0, trees.Ids);
            da.SetDataTree(1, trees.Types);
            da.SetDataTree(2, trees.Names);
            da.SetDataTree(3, trees.Prefixes);
            da.SetDataTree(4, trees.UiIds);
            da.SetDataTree(5, trees.CustomUiIds);
            da.SetDataTree(6, trees.CustomNames);
            da.SetDataTree(7, trees.IsIndependent);
            da.SetDataTree(8, trees.SourceIds);
            da.SetDataTree(9, trees.ChildCounts);
            da.SetData(10, ToWrapper(root?["navigatorItemId"]));
        }

        private class Trees
        {
            public readonly DataTree<NavigatorGuidWrapper> Ids = new DataTree<NavigatorGuidWrapper>();
            public readonly DataTree<string> Types = new DataTree<string>();
            public readonly DataTree<string> Names = new DataTree<string>();
            public readonly DataTree<string> Prefixes = new DataTree<string>();
            public readonly DataTree<string> UiIds = new DataTree<string>();
            public readonly DataTree<object> CustomUiIds = new DataTree<object>();
            public readonly DataTree<object> CustomNames = new DataTree<object>();
            public readonly DataTree<object> IsIndependent = new DataTree<object>();
            public readonly DataTree<NavigatorGuidWrapper> SourceIds = new DataTree<NavigatorGuidWrapper>();
            public readonly DataTree<object> ChildCounts = new DataTree<object>();
        }

        private static NavigatorGuidWrapper ToWrapper(
            JToken idObject)
        {
            var id = idObject?.ToObject<NavigatorGuid>();
            return id == null
                ? null
                : new NavigatorGuidWrapper { Id = id };
        }

        private static void AddChildren(
            JToken item,
            GH_Path path,
            Trees trees)
        {
            if (!(item?["children"] is JArray children))
            {
                return;
            }

            for (var childIndex = 0; childIndex < children.Count; childIndex++)
            {
                var child = children[childIndex]?["navigatorItem"];
                if (child == null)
                {
                    continue;
                }

                trees.Ids.Add(ToWrapper(child["navigatorItemId"]), path);
                trees.Types.Add(child["type"]?.ToString(), path);
                trees.Names.Add(child["name"]?.ToString(), path);
                trees.Prefixes.Add(child["prefix"]?.ToString(), path);
                trees.UiIds.Add(child["uiId"]?.ToString(), path);
                trees.CustomUiIds.Add(JsonOutputHelp.Scalar(child, "customUiId"), path);
                trees.CustomNames.Add(JsonOutputHelp.Scalar(child, "customName"), path);
                trees.IsIndependent.Add(JsonOutputHelp.Scalar(child, "isIndependent"), path);
                trees.SourceIds.Add(ToWrapper(child["sourceNavigatorItemId"]), path);
                trees.ChildCounts.Add(JsonOutputHelp.Scalar(child, "childCount"), path);

                AddChildren(
                    child,
                    path.AppendElement(childIndex),
                    trees);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetNavigatorItemTree;

        public override Guid ComponentGuid =>
            new Guid("aee3e82e-4a8d-476f-9aba-8df4cb2971c8");
    }
}
