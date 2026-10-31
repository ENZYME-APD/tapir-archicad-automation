using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    // The item fields CreateAddOnViewpoints and ChangeAddOnViewpoints share:
    // displayId, displayName, viewSettingsFlags and data, each from a list
    // input that holds 0 (not given), 1 (the same for all) or one value per item.
    public class AddOnViewpointFields
    {
        public List<string> DisplayIds = new List<string>();
        public List<string> DisplayNames = new List<string>();
        public List<double> ViewSettingsFlags = new List<double>();
        public List<string> Data = new List<string>();

        public const string ListDoc =
            "Input only 1 to use the same value for all items. Optional.";

        public static void AddInputs(
            Component component)
        {
            component.InTexts(
                "DisplayIds",
                "The ID shown for each item in the Project Map. " + ListDoc);

            component.InTexts(
                "DisplayNames",
                "The name shown for each item in the Project Map. " + ListDoc);

            component.InNumbers(
                "ViewSettingsFlags",
                "The view settings flags of each item (leaf nodes only), as Archicad stores them: a bit field, " +
                "an integer from 0 to 4294967295. " + ListDoc);

            component.InTexts(
                "Data",
                "Data to attach to each item, base64 encoded. It replaces the attached data; an empty string " +
                "removes it. " + ListDoc);
        }

        // Reads the four list inputs starting at firstIndex and checks their sizes.
        public static AddOnViewpointFields Read(
            GH_ActiveObject component,
            IGH_DataAccess da,
            int firstIndex,
            string itemsName,
            int itemCount)
        {
            var fields = new AddOnViewpointFields();

            da.GetDataList(firstIndex, fields.DisplayIds);
            da.GetDataList(firstIndex + 1, fields.DisplayNames);
            da.GetDataList(firstIndex + 2, fields.ViewSettingsFlags);
            da.GetDataList(firstIndex + 3, fields.Data);

            if (!NavigatorComponentHelp.CheckCount(component, "DisplayIds", fields.DisplayIds.Count, itemsName, itemCount) ||
                !NavigatorComponentHelp.CheckCount(component, "DisplayNames", fields.DisplayNames.Count, itemsName, itemCount) ||
                !NavigatorComponentHelp.CheckCount(component, "ViewSettingsFlags", fields.ViewSettingsFlags.Count, itemsName, itemCount) ||
                !NavigatorComponentHelp.CheckCount(component, "Data", fields.Data.Count, itemsName, itemCount))
            {
                return null;
            }

            foreach (var flags in fields.ViewSettingsFlags)
            {
                if (flags < 0 ||
                    flags > uint.MaxValue ||
                    Math.Floor(flags) != flags)
                {
                    component.AddError(
                        "ViewSettingsFlags must be integers from 0 to 4294967295.");
                    return null;
                }
            }

            return fields;
        }

        // Adds the fields given for the item at index to item.
        public void AddTo(
            JObject item,
            int index)
        {
            if (DisplayIds.Count > 0)
            {
                item["displayId"] = NavigatorComponentHelp.GetAt(DisplayIds, index);
            }

            if (DisplayNames.Count > 0)
            {
                item["displayName"] = NavigatorComponentHelp.GetAt(DisplayNames, index);
            }

            if (ViewSettingsFlags.Count > 0)
            {
                item["viewSettingsFlags"] = (long)NavigatorComponentHelp.GetAt(ViewSettingsFlags, index);
            }

            if (Data.Count > 0)
            {
                item["data"] = NavigatorComponentHelp.GetAt(Data, index) ?? "";
            }
        }

        public static AddOnViewpointGuid IdOf(
            GH_ObjectWrapper wrapper)
        {
            return AddOnViewpointGuid.CreateFromWrapper(wrapper);
        }
    }
}
