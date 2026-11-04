using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.TeamworkComponents
{
    public class GetTeamworkRightsOfObjectsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetTeamworkRightsOfObjects";

        public GetTeamworkRightsOfObjectsComponent()
            : base(
                "GetTeamworkRightsOfObjects",
                "Returns for each given object, for example an element, whether the current Teamwork user " +
                "has the right to create objects of its kind and to delete or modify it.",
                GroupNames.Teamwork)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ObjectGuids",
                "The objects to check, by guid, for example element guids.");
        }

        protected override void AddOutputs()
        {
            OutBooleans(
                "CanCreate",
                "Whether the current user has the right to create objects of the kind of each object " +
                "(null for failed items).");

            OutBooleans(
                "CanDeleteModify",
                "Whether the current user has the right to delete or modify each object (null for failed items).");

            OutErrorMessages(
                "Error message of each object (empty when its rights were read).");
        }

        // The guid of any identifier the components hand out: an element id,
        // a wrapper of another kind of id (its text is the guid) or a guid text.
        private static string GuidOf(
            GH_ObjectWrapper wrapper)
        {
            var elementGuid = GuidObject<ElementGuid>.CreateFromWrapper(wrapper);
            if (elementGuid != null)
            {
                return elementGuid.Guid;
            }

            return GuidObject<ElementGuid>.FromString(wrapper?.Value?.ToString())?.Guid;
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> wrappers))
            {
                return;
            }

            var objectIds = new JArray();
            foreach (var wrapper in wrappers)
            {
                var guid = GuidOf(wrapper);
                if (guid == null)
                {
                    this.AddError("Invalid object identifier in the ObjectGuids input.");
                    return;
                }

                objectIds.Add(new JObject { ["guid"] = guid });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["objectIds"] = objectIds },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var canCreate = new List<object>();
            var canDeleteModify = new List<object>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "rights"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    canCreate.Add(null);
                    canDeleteModify.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                canCreate.Add(item.Value<bool?>("canCreate"));
                canDeleteModify.Add(item.Value<bool?>("canDeleteModify"));
                errorMessages.Add("");
            }

            da.SetDataList(
                0,
                canCreate);

            da.SetDataList(
                1,
                canDeleteModify);

            da.SetDataList(
                2,
                errorMessages);
        }

        // Shares the icon of GetTeamworkStatus until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetTeamworkStatus;

        public override Guid ComponentGuid =>
            new Guid("c55e2517-1723-4588-a0fa-2ca0db245808");
    }
}
