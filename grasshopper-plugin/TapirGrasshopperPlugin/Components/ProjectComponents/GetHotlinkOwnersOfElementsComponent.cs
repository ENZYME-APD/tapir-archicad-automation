using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetHotlinkOwnersOfElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetHotlinkOwnersOfElements";

        public GetHotlinkOwnersOfElementsComponent()
            : base(
                "GetHotlinkOwnersOfElements",
                "Tells for each element whether it comes from a hotlinked module, and returns the hotlink element " +
                "that contains it and the hotlink instance it belongs to. Available from Archicad 27.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to check.");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "IsInHotlink",
                "True when the element comes from a hotlinked module (null where the check failed).");

            OutGenerics(
                "HotlinkElementGuids",
                "The hotlink element which contains the element (null when there is none).");

            OutGenerics(
                "HotlinkInstanceGuids",
                "The hotlink instance the element belongs to (null when there is none). " +
                "Feed it to GetHotlinkProxyElementTable.");

            OutErrorMessages(
                "Error message of each element (empty when the check succeeded).");
        }

        private static ElementGuidWrapper ElementIdOf(
            JToken item,
            string key)
        {
            var guid = JsonOutputHelp.WrappedGuidOf(
                item,
                key);
            return guid == null
                ? null
                : new ElementGuidWrapper
                {
                    ElementId = new ElementGuid { Guid = guid }
                };
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

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(elements),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var isInHotlink = new List<bool?>();
            var hotlinkElementIds = new List<ElementGuidWrapper>();
            var hotlinkInstanceIds = new List<ElementGuidWrapper>();
            var errors = new List<string>();
            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "hotlinkOwners"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    isInHotlink.Add(null);
                    hotlinkElementIds.Add(null);
                    hotlinkInstanceIds.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                isInHotlink.Add(item.Value<bool?>("isInHotlink") ?? false);
                hotlinkElementIds.Add(
                    ElementIdOf(
                        item,
                        "hotlinkElementId"));
                hotlinkInstanceIds.Add(
                    ElementIdOf(
                        item,
                        "hotlinkInstanceId"));
                errors.Add("");
            }

            da.SetDataList(0, isInHotlink);
            da.SetDataList(1, hotlinkElementIds);
            da.SetDataList(2, hotlinkInstanceIds);
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetHotlinkOwnersOfElements;

        public override Guid ComponentGuid =>
            new Guid("2c9125d2-0864-40d3-94ff-007c7ae01f5c");
    }
}
