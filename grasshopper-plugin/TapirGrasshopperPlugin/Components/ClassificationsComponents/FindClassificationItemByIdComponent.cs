using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

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
                "The Guid of the classification system.");

            InText(
                "ClassificationItemId",
                "Classification Item Id to find.");
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
            AddAsSource<ClassificationSystemValueList>(
                document,
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
            if (!da.TryCreate(
                    0,
                    out ClassificationGuid classificationSystemId))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string classificationItemId))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { classificationSystemId },
                    ToArchicad,
                    JHelp.Deserialize<AllClassificationItemsInSystem>,
                    out AllClassificationItemsInSystem response))
            {
                return;
            }

            var found = FindClassificationItemInTree(
                response.ClassificationItems,
                classificationItemId.ToLower());

            if (found == null)
            {
                this.AddError("ClassificationItem not found!");
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