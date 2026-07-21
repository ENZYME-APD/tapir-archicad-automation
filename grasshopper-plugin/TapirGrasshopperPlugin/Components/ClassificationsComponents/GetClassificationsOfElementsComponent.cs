using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    public class GetClassificationsOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetClassificationsOfElements";

        public GetClassificationsOfElementsComponent()
            : base(
                "GetClassificationsOfElements",
                "Get the classifications of the given elements in the given classification systems. " +
                "Outputs have one branch per queried element, with one item per queried classification system. " +
                "The item guid is null when the element is unclassified in that system.",
                GroupNames.Classifications)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");

            InGenerics(
                "ClassificationSystemGuids",
                "Identifiers of the classification systems to query.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ClassificationSystemGuids",
                "Classification system of each classification (one branch per element).");

            OutGenericTree(
                "ClassificationItemGuids",
                "Classification item of each classification (one branch per element; null when unclassified).");

            OutTextTree(
                "ClassificationErrorMessages",
                "Error message for each element and system (one branch per element; empty when successful).");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried element (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
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

            var parameters = JObject.FromObject(elements);
            parameters["classificationSystemIds"] = JArray.FromObject(systemIds);

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
                "elementClassifications");
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

                        var classificationId = idOrError["classificationId"];
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
                        classificationErrors.Add("", path);
                    }
                }
            }

            da.SetDataTree(0, classificationSystemGuids);
            da.SetDataTree(1, classificationItemGuids);
            da.SetDataTree(2, classificationErrors);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetClassificationsOfElements;

        public override Guid ComponentGuid =>
            new Guid("44498153-a3d4-4b72-91ce-9cdbc236af0c");
    }
}
