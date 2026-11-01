using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetAddOnViewpointsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAddOnViewpoints";

        public GetAddOnViewpointsComponent()
            : base(
                "GetAddOnViewpoints",
                "Returns the details of the given add-on viewpoint items: their parent, type (Root, Group or " +
                "Leaf), ID, name, view settings flags and attached data (base64 encoded). The output is aligned " +
                "with the input array; an item that cannot be read gets an error. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ViewpointIds",
                "Identifiers of the add-on viewpoint items.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ParentViewpointIds",
                "The root or group each item is in (null for a root and for a failed item).");

            OutTexts(
                "ItemTypes",
                "Whether each item is a Root, a Group or a Leaf node (or Undefined).");

            OutTexts(
                "DisplayIds",
                "The ID shown for each item in the Project Map.");

            OutTexts(
                "DisplayNames",
                "The name shown for each item in the Project Map.");

            OutNumberList(
                "ViewSettingsFlags",
                "The view settings flags of each item, as Archicad stores them (a bit field).");

            OutTexts(
                "Data",
                "The data the add-on attached to each item, base64 encoded (empty when there is none).");

            OutErrorMessages(
                "Error message of each item (empty when it succeeded).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out AddOnViewpointsObject viewpoints))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    JObject.FromObject(viewpoints),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var parentIds = new List<AddOnViewpointGuid>();
            var itemTypes = new List<string>();
            var displayIds = new List<string>();
            var displayNames = new List<string>();
            var viewSettingsFlags = new List<object>();
            var data = new List<string>();
            var errorMessages = new List<string>();

            foreach (var item in JsonOutputHelp.Items(
                         response,
                         "viewpoints"))
            {
                if (JsonOutputHelp.IsError(item))
                {
                    parentIds.Add(null);
                    itemTypes.Add(null);
                    displayIds.Add(null);
                    displayNames.Add(null);
                    viewSettingsFlags.Add(null);
                    data.Add(null);
                    errorMessages.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                parentIds.Add(item["parentViewpointId"]?.ToObject<AddOnViewpointGuid>());
                itemTypes.Add(item["itemType"]?.ToString());
                displayIds.Add(item["displayId"]?.ToString());
                displayNames.Add(item["displayName"]?.ToString());
                viewSettingsFlags.Add(item.Value<double?>("viewSettingsFlags"));
                data.Add(item["data"]?.ToString());
                errorMessages.Add("");
            }

            da.SetDataList(0, parentIds);
            da.SetDataList(1, itemTypes);
            da.SetDataList(2, displayIds);
            da.SetDataList(3, displayNames);
            da.SetDataList(4, viewSettingsFlags);
            da.SetDataList(5, data);
            da.SetDataList(6, errorMessages);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAddOnViewpoints;

        public override Guid ComponentGuid =>
            new Guid("135caded-c6ad-4b70-8d40-4834d2725a74");
    }
}
