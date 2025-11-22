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

        protected override void AddOutputs()
        {
            OutTexts(
                "ClassificationSystemGuid",
                "Found Classification System Guid.");
            OutTexts(
                "ClassificationSystemNameAndVersion",
                "Found Classification System name and version.");

            OutTexts(
                "ClassificationItemGuid",
                "Found ClassificationItem Guid.");

            OutTexts(
                "ClassificationItemDisplayId",
                "Found ClassificationItem display id.");

            OutTexts(
                "ClassificationItemFullDisplayId",
                "Found ClassificationItem full display id.");

            OutTexts(
                "ClassificationItemName",
                "Found ClassificationItem name.");

            OutTexts(
                "ClassificationItemPath",
                "Path to ClassificationItem.");
        }


        private List<Tuple<ClassificationItemDetailsObj, string>>
            GetAllClassificationItemFromTree(
                List<ClassificationItemObj> tree,
                string pathToRoot)
        {
            List<Tuple<ClassificationItemDetailsObj, string>> list =
                new List<Tuple<ClassificationItemDetailsObj, string>>();

            foreach (ClassificationItemObj item in tree)
            {
                string path = pathToRoot + '/' + item.ClassificationItem.Id;
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
                itemsPerSystems =
                    new Dictionary<ClassificationSystemDetailsObj,
                        List<Tuple<ClassificationItemDetailsObj, string>>>();


            if (!GetConvertedResponse(
                    CommandName,
                    null,
                    out AllClassificationSystems classificationSystems))
            {
                return;
            }


            foreach (ClassificationSystemDetailsObj system in
                     classificationSystems.ClassificationSystems)
            {
                ClassificationSystemObj classificationSystem =
                    new ClassificationSystemObj()
                    {
                        ClassificationSystemId =
                            system.ClassificationSystemId
                    };

                JObject classificationSystemObj =
                    JObject.FromObject(classificationSystem);

                if (!GetConvertedResponse(
                        CommandName,
                        classificationSystemObj,
                        out AllClassificationItemsInSystem
                            classificationItemsInSystem)) { return; }

                List<Tuple<ClassificationItemDetailsObj, string>>
                    itemsInSystem = GetAllClassificationItemFromTree(
                        classificationItemsInSystem.ClassificationItems,
                        system.ToString());

                itemsPerSystems.Add(
                    system,
                    itemsInSystem);
            }

            List<string> systemIds = new List<string>();
            List<string> systemNamesAndVersions = new List<string>();
            List<string> itemIds = new List<string>();
            List<string> itemDisplayIds = new List<string>();
            List<string> itemNames = new List<string>();
            List<string> itemFullDisplayIds = new List<string>();
            List<string> itemPaths = new List<string>();

            foreach (KeyValuePair<ClassificationSystemDetailsObj,
                             List<Tuple<ClassificationItemDetailsObj, string>>>
                         itemsInSystem in itemsPerSystems)
            {
                ClassificationSystemDetailsObj system = itemsInSystem.Key;
                foreach (Tuple<ClassificationItemDetailsObj, string>
                             itemDetailAndPath in itemsInSystem.Value)
                {
                    ClassificationItemDetailsObj itemDetail =
                        itemDetailAndPath.Item1;
                    string itemPath = itemDetailAndPath.Item2;

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
            new Guid("46a81cf1-e043-4cb7-b587-c2a6d3349bd8");
    }
}