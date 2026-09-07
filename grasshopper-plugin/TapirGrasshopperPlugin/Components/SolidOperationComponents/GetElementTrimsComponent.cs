using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.SolidOperationComponents
{
    public class GetElementTrimsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetElementTrims";

        public GetElementTrimsComponent()
            : base(
                "GetElementTrims",
                "Get the roofs and shells trimming each queried element, and the elements it trims. " +
                "All outputs have one branch per queried element.",
                GroupNames.ElementSolidOperations)
        {
        }

        protected override string ResponseArrayKey => "elementTrims";

        protected override void AddOutputs()
        {
            OutGenericTree(
                "TrimmedByGuids",
                "The roofs and shells trimming the queried element.");

            OutTextTree(
                "TrimTypes",
                "Trim type of each trim the queried element is trimmed by.");

            OutGenericTree(
                "TrimsGuids",
                "The elements the queried element trims, when it is a roof or a shell.");

            OutErrorMessages(
                "Error message of each queried element (empty when it could be read); " +
                "its branches above stay empty.");
        }

        private static ElementGuidWrapper ElementIdFromGuidObject(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }
            return new ElementGuidWrapper
            {
                ElementId = new ElementGuid { Guid = guid }
            };
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var trimmedByGuids = new DataTree<object>();
            var trimTypes = new DataTree<object>();
            var trimsGuids = new DataTree<object>();
            var errorMessages = new List<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                trimmedByGuids.EnsurePath(path);
                trimTypes.EnsurePath(path);
                trimsGuids.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }
                errorMessages.Add("");

                if (item["trimmedBy"] is JArray trimmedBy)
                {
                    foreach (var trim in trimmedBy)
                    {
                        trimmedByGuids.Add(
                            ElementIdFromGuidObject(trim["elementId"]),
                            path);
                        trimTypes.Add(
                            JsonOutputHelp.Scalar(trim, "trimType"),
                            path);
                    }
                }

                if (item["trims"] is JArray trims)
                {
                    foreach (var trim in trims)
                    {
                        trimsGuids.Add(
                            ElementIdFromGuidObject(trim["elementId"] ?? trim),
                            path);
                    }
                }
            }

            da.SetDataTree(0, trimmedByGuids);
            da.SetDataTree(1, trimTypes);
            da.SetDataTree(2, trimsGuids);
            da.SetDataList(3, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetElementTrims;

        public override Guid ComponentGuid =>
            new Guid("847f24da-cbc7-aff4-84b4-2559980f0d7e");
    }
}
