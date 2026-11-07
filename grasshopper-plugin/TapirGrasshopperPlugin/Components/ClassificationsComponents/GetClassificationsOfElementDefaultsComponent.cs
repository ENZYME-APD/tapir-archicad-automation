using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationsOfElementDefaultsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetClassificationsOfElementDefaults";

        public GetClassificationsOfElementDefaultsComponent()
            : base(
                "GetClassificationsOfElementDefaults",
                "Get the classifications of the tool defaults (the settings new elements get) of element types " +
                "in the given classification systems. " +
                "Outputs have one branch per queried element type, with one item per queried classification system. " +
                "The item guid is null when the default is unclassified in that system.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "Element types whose defaults to query.");

            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification (one branch per element type).");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification (one branch per element type; null when unclassified).");

            OutTextTree(
                "ClassificationErrorMessages",
                "Error message for each element type and system (one branch per element type; empty when successful).");

            OutTexts(
                "ErrorMessages",
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

            if (!da.TryGetList(
                    1,
                    out List<GH_ObjectWrapper> systemWrappers))
            {
                return;
            }

            var systemIds = new List<ClassificationSystemGuidWrapper>();
            foreach (var wrapper in systemWrappers)
            {
                var id = GuidObject<ClassificationGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError(
                        "Invalid classification system identifier in the ClassificationSystemGuids input.");
                    return;
                }
                systemIds.Add(
                    new ClassificationSystemGuidWrapper { ClassificationSystemId = id });
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes),
                ["classificationSystemIds"] = JArray.FromObject(systemIds)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var classificationSystemGuids = new DataTree<object>();
            var classificationItemGuids = new DataTree<object>();
            var classificationErrors = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementDefaultClassifications");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                classificationSystemGuids.EnsurePath(path);
                classificationItemGuids.EnsurePath(path);
                classificationErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");

                if (item["classificationIds"] is JArray classificationIds)
                {
                    foreach (var idOrError in classificationIds)
                    {
                        if (JsonOutputHelp.IsError(idOrError))
                        {
                            classificationSystemGuids.Add(null, path);
                            classificationItemGuids.Add(null, path);
                            classificationErrors.Add(
                                JsonOutputHelp.ErrorMessage(idOrError),
                                path);
                            continue;
                        }

                        // The command answers with the system and item ids
                        // directly in each item; also accept them wrapped in
                        // a classificationId object, as the schema describes.
                        var classificationId =
                            idOrError["classificationId"] ?? idOrError;
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
                            itemGuid == null || IsNullGuid(itemGuid)
                                ? null
                                : new ClassificationItemGuidWrapper
                                {
                                    ClassificationItemId =
                                        new ClassificationGuid { Guid = itemGuid }
                                },
                            path);
                        classificationErrors.Add("", path);
                    }
                }
            }

            da.SetDataTree(0, classificationSystemGuids);
            da.SetDataTree(1, classificationItemGuids);
            da.SetDataTree(2, classificationErrors);
            da.SetDataList(3, errors);
        }

        // An unclassified default comes back with the null guid as its item.
        private static bool IsNullGuid(
            string guid)
        {
            return Guid.TryParse(guid, out var parsed) && parsed == Guid.Empty;
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("3f9a6d12-b874-4e5c-a2d9-61c0e8f47b95");
    }
}
