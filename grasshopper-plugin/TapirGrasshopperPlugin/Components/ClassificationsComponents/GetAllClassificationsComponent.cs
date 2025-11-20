using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Element;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetAllClassificationsComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Get all Classifications in the given Classification System.";

        public override string CommandName => "GetAllClassificationSystems";

        public GetAllClassificationsComponent()
            : base(
                "All Classifications in the given Classification System.",
                "AllClassifications",
                Doc,
                GroupNames.Classifications)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "ClassificationSystemGuid",
                "SystemGuid",
                "Found Classification System Guid.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationSystemNameAndVersion",
                "SystemNameAndVersion",
                "Found Classification System name and version.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationItemGuid",
                "ItemGuid",
                "Found ClassificationItem Guid.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationItemDisplayId",
                "ItemDisplayId",
                "Found ClassificationItem display id.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationItemFullDisplayId",
                "ItemFullDisplayId",
                "Found ClassificationItem full display id.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationItemName",
                "ItemName",
                "Found ClassificationItem name.",
                GH_ParamAccess.list);
            pManager.AddTextParameter(
                "ClassificationItemPath",
                "ItemPath",
                "Path to ClassificationItem.",
                GH_ParamAccess.list);
        }

        private List<Tuple<ClassificationItemDetailsObj, string>>
            GetAllClassificationItemFromTree(
                List<ClassificationItemObj> tree,
                string pathToRoot)
        {
            List<Tuple<ClassificationItemDetailsObj, string>> list = new();
            foreach (var item in tree)
            {
                var path = pathToRoot + '/' + item.ClassificationItem.Id;
                list.Add(
                    new Tuple<ClassificationItemDetailsObj, string>(
                        item.ClassificationItem,
                        path));
                if (item.ClassificationItem.Children != null)
                {
                    list.AddRange(
                        GetAllClassificationItemFromTree(
                            item.ClassificationItem.Children,
                            path));
                }
            }

            return list;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            Dictionary<ClassificationSystemDetailsObj,
                    List<Tuple<ClassificationItemDetailsObj, string>>>
                itemsPerSystems = new();


            if (!GetConvertedResponse(
                    CommandName,
                    null,
                    out AllClassificationSystems classificationSystems))
            {
                return;
            }


            foreach (var system in classificationSystems.ClassificationSystems)
            {
                ClassificationSystemObj classificationSystem = new()
                {
                    ClassificationSystemId = system.ClassificationSystemId
                };

                var classificationSystemObj =
                    JObject.FromObject(classificationSystem);

                if (!GetConvertedResponse(
                        CommandName,
                        classificationSystemObj,
                        out AllClassificationItemsInSystem
                            classificationItemsInSystem)) { return; }

                var itemsInSystem = GetAllClassificationItemFromTree(
                    classificationItemsInSystem.ClassificationItems,
                    system.ToString());

                itemsPerSystems.Add(
                    system,
                    itemsInSystem);
            }

            List<string> systemIds = new();
            List<string> systemNamesAndVersions = new();
            List<string> itemIds = new();
            List<string> itemDisplayIds = new();
            List<string> itemNames = new();
            List<string> itemFullDisplayIds = new();
            List<string> itemPaths = new();
            foreach (var itemsInSystem in itemsPerSystems)
            {
                var system = itemsInSystem.Key;
                foreach (var itemDetailAndPath in itemsInSystem.Value)
                {
                    var itemDetail = itemDetailAndPath.Item1;
                    var itemPath = itemDetailAndPath.Item2;

                    systemIds.Add(system.ClassificationSystemId.Guid);
                    systemNamesAndVersions.Add(system.ToString());
                    itemIds.Add(itemDetail.ClassificationItemId.Guid);
                    itemDisplayIds.Add(itemDetail.Id);
                    itemFullDisplayIds.Add(
                        ArchicadUtils.JoinNames(
                            system.ToString(),
                            itemDetail.Id));
                    itemNames.Add(itemDetail.Name);
                    itemPaths.Add(itemPath);
                }
            }

            da.SetDataList(
                0,
                systemIds);
            da.SetDataList(
                1,
                systemNamesAndVersions);
            da.SetDataList(
                2,
                itemIds);
            da.SetDataList(
                3,
                itemDisplayIds);
            da.SetDataList(
                4,
                itemFullDisplayIds);
            da.SetDataList(
                5,
                itemNames);
            da.SetDataList(
                6,
                itemPaths);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.AllClassifications;

        public override Guid ComponentGuid =>
            new("46a81cf1-e043-4cb7-b587-c2a6d3349bd8");
    }
}