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

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetElementSetDataComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetElementSetData";

        public GetElementSetDataComponent()
            : base(
                "GetElementSetData",
                "Get the elements and the custom data of each given element set. Element sets are created by " +
                "add-ons to link elements together; the custom data is returned as Base64 encoded bytes.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementSetGuids",
                "Identifiers of the element sets.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ElementGuids",
                "Elements of each element set (one branch per element set).");

            OutIntegers(
                "DataVersions",
                "Version of the custom data of each element set, as set by the add-on that created it.");

            OutIntegers(
                "PlatformSigns",
                "Platform sign of the custom data of each element set, telling the byte order it was written with.");

            OutTexts(
                "Data",
                "Raw bytes of the custom data of each element set, Base64 encoded (empty when it has no data).");

            OutErrorMessages(
                "Error message for each element set (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> setWrappers))
            {
                return;
            }

            var elementSets = new JArray();
            foreach (var wrapper in setWrappers)
            {
                var id = GuidObject<ElementSetGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid element set identifier in the ElementSetGuids input.");
                    return;
                }
                elementSets.Add(
                    new JObject
                    {
                        ["elementSetId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["elementSets"] = elementSets },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var elementGuids = new DataTree<object>();
            var dataVersions = new List<object>();
            var platformSigns = new List<object>();
            var data = new List<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementSets");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                elementGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    dataVersions.Add(null);
                    platformSigns.Add(null);
                    data.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                if (item["elements"] is JArray elements)
                {
                    foreach (var element in elements)
                    {
                        elementGuids.Add(
                            ElementsStructuredGetterComponent.ElementIdOf(element),
                            path);
                    }
                }

                var userData = item["userData"];
                dataVersions.Add(JsonOutputHelp.Scalar(userData, "dataVersion"));
                platformSigns.Add(JsonOutputHelp.Scalar(userData, "platformSign"));
                data.Add(JsonOutputHelp.Scalar(userData, "data"));
                errors.Add("");
            }

            da.SetDataTree(0, elementGuids);
            da.SetDataList(1, dataVersions);
            da.SetDataList(2, platformSigns);
            da.SetDataList(3, data);
            da.SetDataList(4, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementSetData;

        public override Guid ComponentGuid =>
            new Guid("9e2195d2-3b11-4bee-8ef7-1c0449c62775");
    }
}
