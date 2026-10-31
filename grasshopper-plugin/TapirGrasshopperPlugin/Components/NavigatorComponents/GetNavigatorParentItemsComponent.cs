using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetNavigatorParentItemsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetNavigatorParentItems";

        public GetNavigatorParentItemsComponent()
            : base(
                "GetNavigatorParentItems",
                "Returns the parent of each given navigator item: the folder, subset or layout book item it is " +
                "in. The root of a map has no parent and gets an error.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifiers of the navigator items.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ParentNavigatorItemIds",
                "The identifier of the parent of each navigator item (null for a failed item).");

            OutTexts(
                "ParentTypes",
                "The type of the parent of each navigator item (null for a failed item).");

            OutTexts(
                "ParentNames",
                "The name of the parent of each navigator item (null for a failed item).");

            OutErrorMessages(
                "Error message of each navigator item (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject navigatorItems))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(navigatorItems),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var parentIds = new List<NavigatorGuidWrapper>();
            var parentTypes = new List<string>();
            var parentNames = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "parentItems"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    parentIds.Add(null);
                    parentTypes.Add(null);
                    parentNames.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                parentIds.Add(
                    new NavigatorGuidWrapper
                    {
                        Id = item["navigatorItemId"]?.ToObject<NavigatorGuid>()
                    });
                parentTypes.Add(item["type"]?.ToString());
                parentNames.Add(item["name"]?.ToString());
                errorMessages.Add("");
            }

            da.SetDataList(0, parentIds);
            da.SetDataList(1, parentTypes);
            da.SetDataList(2, parentNames);
            da.SetDataList(3, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetNavigatorParentItems;

        public override Guid ComponentGuid =>
            new Guid("c68e5425-6d00-4fba-8a0f-f8441ccd0b80");
    }
}
