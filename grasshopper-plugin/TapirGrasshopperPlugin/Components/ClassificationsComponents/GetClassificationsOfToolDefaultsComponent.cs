using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationsOfToolDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetClassificationsOfToolDefaults";

        public GetClassificationsOfToolDefaultsComponent()
            : base(
                "GetClassificationsOfToolDefaults",
                "Get every classification of the tool defaults (the settings new elements get) of the given " +
                "element types, in all classification systems at once. Unlike GetClassificationsOfElementDefaults " +
                "it needs no list of classification systems; systems in which a default is unclassified are left " +
                "out. Outputs have one branch per queried element type.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Element types whose tool defaults to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification (one branch per element type).");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification (one branch per element type).");

            OutErrorMessages(
                "Error message for each queried element type (empty when successful).");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elementTypes"] = JArray.FromObject(elementTypes) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var classificationSystemGuids = new DataTree<object>();
            var classificationItemGuids = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "classificationsOfToolDefaults");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                classificationSystemGuids.EnsurePath(path);
                classificationItemGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (!(item["classificationIds"] is JArray classificationIds))
                {
                    continue;
                }

                foreach (var classificationId in classificationIds)
                {
                    var systemGuid = JsonOutputHelp.WrappedGuidOf(
                        classificationId,
                        "classificationSystemId");
                    var itemGuid = JsonOutputHelp.WrappedGuidOf(
                        classificationId,
                        "classificationItemId");

                    classificationSystemGuids.Add(
                        systemGuid == null
                            ? null
                            : new ClassificationSystemGuidWrapper
                            {
                                ClassificationSystemId =
                                    new ClassificationGuid { Guid = systemGuid }
                            },
                        path);
                    classificationItemGuids.Add(
                        itemGuid == null
                            ? null
                            : new ClassificationItemGuidWrapper
                            {
                                ClassificationItemId =
                                    new ClassificationGuid { Guid = itemGuid }
                            },
                        path);
                }
            }

            da.SetDataTree(0, classificationSystemGuids);
            da.SetDataTree(1, classificationItemGuids);
            da.SetDataList(2, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfToolDefaults;

        public override Guid ComponentGuid =>
            new Guid("0c8ed4fa-92ba-4626-9826-e720e39fa888");
    }
}
