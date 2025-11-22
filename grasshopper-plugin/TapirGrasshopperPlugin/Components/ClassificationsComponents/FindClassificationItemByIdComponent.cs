using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class FindClassificationItemByIdComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllClassificationsInSystem";

        public FindClassificationItemByIdComponent()
            : base(
                "FindClassificationItemById",
                "Finds a Classification Item by Id in the given Classification System.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "ClassificationSystemGuid",
                "The Guid of a classification system.");

            InText(
                "ClassificationItemId",
                "Classification Item id to find.");
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "ClassificationItemGuid",
                "Found ClassificationItem Guid.");
        }


        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ClassificationSystemValueList().AddAsSource(
                this,
                0);
        }

        private ClassificationItemDetailsObj FindClassificationItemInTree(
            List<ClassificationItemObj> branch,
            string classificationItemId)
        {
            foreach (var item in branch)
            {
                if (item.ClassificationItem.Id.ToLower() ==
                    classificationItemId)
                {
                    return item.ClassificationItem;
                }

                if (item.ClassificationItem.Children != null)
                {
                    var foundInChildren = FindClassificationItemInTree(
                        item.ClassificationItem.Children,
                        classificationItemId);
                    if (foundInChildren != null)
                    {
                        return foundInChildren;
                    }
                }
            }

            return null;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!ClassificationIdObj.TryCreate(
                    da,
                    0,
                    out ClassificationIdObj systemId))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out string cId))
            {
                return;
            }

            if (!TryGetConvertedResponse(
                    CommandName,
                    new ClassificationSystemObj
                    {
                        ClassificationSystemId = systemId
                    },
                    out AllClassificationItemsInSystem cItems)) { return; }


            var found = FindClassificationItemInTree(
                cItems.ClassificationItems,
                cId.ToLower());

            if (found == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "ClassificationItem is not found.");
            }
            else
            {
                da.SetData(
                    0,
                    found.ClassificationItemId);
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ClassificationById;

        public override Guid ComponentGuid =>
            new Guid("46026689-054d-4164-9d35-ac56150cd733");
    }
}