using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class FindClassificationItemByIdComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Finds a Classification Item by id in the given Classification System.";

        public override string CommandName => "GetAllClassificationsInSystem";

        public FindClassificationItemByIdComponent()
            : base(
                "Find Classification Item By Id",
                "ClassificationById",
                Doc,
                GroupNames.Classifications)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ClassificationSystemGuid",
                "SystemGuid",
                "The Guid of a classification system.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "Classification Item id",
                "ItemId",
                "Classification Item id to find.",
                GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ClassificationItemGuid",
                "ItemGuid",
                "Found ClassificationItem Guid.",
                GH_ParamAccess.item);
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
            var classificationSystemId = ClassificationIdObj.Create(
                da,
                0);
            if (classificationSystemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ClassificationSystemId failed to collect data.");
                return;
            }

            var ClassificationItemId = "";
            if (!da.GetData(
                    1,
                    ref ClassificationItemId))
            {
                return;
            }

            var classificationSystem = new ClassificationSystemObj()
            {
                ClassificationSystemId = classificationSystemId
            };

            if (!GetConvertedResponse(
                    CommandName,
                    classificationSystem,
                    out AllClassificationItemsInSystem
                        classificationItemsInSystem)) { return; }


            ClassificationItemId = ClassificationItemId.ToLower();
            var found = FindClassificationItemInTree(
                classificationItemsInSystem.ClassificationItems,
                ClassificationItemId);

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