using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetAllClassificationsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllClassificationSystems";
        public string LoopCommandName => "GetAllClassificationsInSystem";

        public GetAllClassificationsComponent()
            : base(
                "AllClassifications",
                "Get all Classifications in the given Classification System.",
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
            var list = new List<Tuple<ClassificationItemDetailsObj, string>>();

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
            var itemsPerSystems =
                new Dictionary<ClassificationSystemDetailsObj,
                    List<Tuple<ClassificationItemDetailsObj, string>>>();

            if (!TryGetConvertedValues(
                    CommandName,
                    null,
                    ToArchicad,
                    JHelp.Deserialize<AllClassificationSystems>,
                    out AllClassificationSystems response))
            {
                return;
            }

            foreach (var system in response.ClassificationSystems)
            {
                if (!TryGetConvertedValues(
                        LoopCommandName,
                        new
                        {
                            classificationSystemId =
                                system.ClassificationSystemId
                        },
                        ToArchicad,
                        JHelp.Deserialize<AllClassificationItemsInSystem>,
                        out AllClassificationItemsInSystem
                            classificationItemsInSystem))
                {
                    return;
                }

                List<Tuple<ClassificationItemDetailsObj, string>>
                    itemsInSystem = GetAllClassificationItemFromTree(
                        classificationItemsInSystem.ClassificationItems,
                        system.ToString());

                itemsPerSystems.Add(
                    system,
                    itemsInSystem);
            }

            var systemIds = new List<string>();
            var systemNamesAndVersions = new List<string>();
            var itemIds = new List<string>();
            var itemDisplayIds = new List<string>();
            var itemNames = new List<string>();
            var itemFullDisplayIds = new List<string>();
            var itemPaths = new List<string>();

            foreach (var itemsInSystem in itemsPerSystems)
            {
                ClassificationSystemDetailsObj system = itemsInSystem.Key;

                foreach (var tuple in itemsInSystem.Value)
                {
                    var detail = tuple.Item1;

                    systemIds.Add(system.ClassificationSystemId.Guid);
                    systemNamesAndVersions.Add(system.ToString());
                    itemIds.Add(detail.ClassificationItemId.Guid);
                    itemDisplayIds.Add(detail.Id);
                    itemFullDisplayIds.Add(
                        StringHelp.Join(
                            system.ToString(),
                            detail.Id));
                    itemNames.Add(detail.Name);
                    itemPaths.Add(tuple.Item2);
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