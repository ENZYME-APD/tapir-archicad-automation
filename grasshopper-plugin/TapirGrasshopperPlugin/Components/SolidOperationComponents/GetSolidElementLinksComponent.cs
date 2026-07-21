using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class GetSolidElementLinksComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetSolidElementLinks";

        public GetSolidElementLinksComponent()
            : base(
                "GetSolidElementLinks",
                "Get solid element operation links for each queried element, grouped by role (target or operator). " +
                "All outputs have one branch per queried element.",
                GroupNames.Elements)
        {
        }

        protected override string ResponseArrayKey => "solidLinks";

        protected override void AddOutputs()
        {
            OutGenericTree(
                "TargetOperatorGuids",
                "Operator element of each link where the queried element is the target.");

            OutTextTree(
                "TargetOperations",
                "Operation of each link where the queried element is the target.");

            OutBooleanTree(
                "TargetInheritAttributes",
                "True if the link inherits the operator attributes (queried element is the target).");

            OutBooleanTree(
                "TargetSkipPolygonHoles",
                "True if the link skips polygon holes (queried element is the target).");

            OutGenericTree(
                "OperatorTargetGuids",
                "Target element of each link where the queried element is the operator.");

            OutTextTree(
                "OperatorOperations",
                "Operation of each link where the queried element is the operator.");

            OutBooleanTree(
                "OperatorInheritAttributes",
                "True if the link inherits the operator attributes (queried element is the operator).");

            OutBooleanTree(
                "OperatorSkipPolygonHoles",
                "True if the link skips polygon holes (queried element is the operator).");
        }

        private static ElementGuidWrapper ElementIdFromGuidObject(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }
            return new ElementGuidWrapper
            {
                ElementId = new ElementGuid { Guid = guid }
            };
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var targetOperatorGuids = new DataTree<object>();
            var targetOperations = new DataTree<object>();
            var targetInheritAttributes = new DataTree<object>();
            var targetSkipPolygonHoles = new DataTree<object>();
            var operatorTargetGuids = new DataTree<object>();
            var operatorOperations = new DataTree<object>();
            var operatorInheritAttributes = new DataTree<object>();
            var operatorSkipPolygonHoles = new DataTree<object>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                targetOperatorGuids.EnsurePath(path);
                targetOperations.EnsurePath(path);
                targetInheritAttributes.EnsurePath(path);
                targetSkipPolygonHoles.EnsurePath(path);
                operatorTargetGuids.EnsurePath(path);
                operatorOperations.EnsurePath(path);
                operatorInheritAttributes.EnsurePath(path);
                operatorSkipPolygonHoles.EnsurePath(path);

                if (item["solidLinksWithTheGivenTarget"] is JArray targetLinks)
                {
                    foreach (var link in targetLinks)
                    {
                        targetOperatorGuids.Add(
                            ElementIdFromGuidObject(link["operatorId"]),
                            path);
                        targetOperations.Add(
                            JsonOutputHelp.Scalar(link, "operation"),
                            path);
                        targetInheritAttributes.Add(
                            JsonOutputHelp.Scalar(link["linkFlags"], "inheritOperatorAttributes"),
                            path);
                        targetSkipPolygonHoles.Add(
                            JsonOutputHelp.Scalar(link["linkFlags"], "skipPolygonHoles"),
                            path);
                    }
                }

                if (item["solidLinksWithTheGivenOperator"] is JArray operatorLinks)
                {
                    foreach (var link in operatorLinks)
                    {
                        operatorTargetGuids.Add(
                            ElementIdFromGuidObject(link["targetId"]),
                            path);
                        operatorOperations.Add(
                            JsonOutputHelp.Scalar(link, "operation"),
                            path);
                        operatorInheritAttributes.Add(
                            JsonOutputHelp.Scalar(link["linkFlags"], "inheritOperatorAttributes"),
                            path);
                        operatorSkipPolygonHoles.Add(
                            JsonOutputHelp.Scalar(link["linkFlags"], "skipPolygonHoles"),
                            path);
                    }
                }
            }

            da.SetDataTree(0, targetOperatorGuids);
            da.SetDataTree(1, targetOperations);
            da.SetDataTree(2, targetInheritAttributes);
            da.SetDataTree(3, targetSkipPolygonHoles);
            da.SetDataTree(4, operatorTargetGuids);
            da.SetDataTree(5, operatorOperations);
            da.SetDataTree(6, operatorInheritAttributes);
            da.SetDataTree(7, operatorSkipPolygonHoles);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSolidElementLinks;

        public override Guid ComponentGuid =>
            new Guid("d7c54912-2b06-4c8c-9535-35d6c6a25831");
    }
}
