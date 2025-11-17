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
        public GetAllClassificationsComponent()
            : base(
                "All Classifications in the given Classification System.",
                "AllClassifications",
                "Get all Classifications in the given Classification System.",
                "Classifications")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
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
            var list = new List<Tuple<ClassificationItemDetailsObj, string>>();
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
            IGH_DataAccess DA)
        {
            var response = SendArchicadCommand(
                "GetAllClassificationSystems",
                null);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var itemsPerSystems =
                new Dictionary<ClassificationSystemDetailsObj,
                    List<Tuple<ClassificationItemDetailsObj, string>>>();
            var classificationSystems =
                response.Result.ToObject<AllClassificationSystems>();
            foreach (var system in classificationSystems.ClassificationSystems)
            {
                var classificationSystem = new ClassificationSystemObj()
                {
                    ClassificationSystemId = system.ClassificationSystemId
                };

                var classificationSystemObj =
                    JObject.FromObject(classificationSystem);
                response = SendArchicadCommand(
                    "GetAllClassificationsInSystem",
                    classificationSystemObj);
                if (!response.Succeeded)
                {
                    AddRuntimeMessage(
                        GH_RuntimeMessageLevel.Error,
                        response.GetErrorMessage());
                    return;
                }

                var classificationItemsInSystem = response.Result
                    .ToObject<AllClassificationItemsInSystem>();
                var itemsInSystem = GetAllClassificationItemFromTree(
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

            DA.SetDataList(
                0,
                systemIds);
            DA.SetDataList(
                1,
                systemNamesAndVersions);
            DA.SetDataList(
                2,
                itemIds);
            DA.SetDataList(
                3,
                itemDisplayIds);
            DA.SetDataList(
                4,
                itemFullDisplayIds);
            DA.SetDataList(
                5,
                itemNames);
            DA.SetDataList(
                6,
                itemPaths);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.AllClassifications;

        public override Guid ComponentGuid =>
            new Guid("46a81cf1-e043-4cb7-b587-c2a6d3349bd8");
    }
}