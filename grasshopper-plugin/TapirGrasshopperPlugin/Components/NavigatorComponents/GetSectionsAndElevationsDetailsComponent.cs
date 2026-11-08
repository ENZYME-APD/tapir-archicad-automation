using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetSectionsAndElevationsDetailsComponent : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetSectionsAndElevationsDetails";

        public GetSectionsAndElevationsDetailsComponent()
            : base(
                "GetSectionsAndElevationsDetails",
                "Returns the details of Section and Elevation elements: their name, reference ID, settings " +
                "(SetSectionsAndElevationsDetails takes them back) and read-only state.",
                GroupNames.Navigator)
        {
        }

        protected override string ResponseArrayKey => "detailsOfElements";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Sections or Elevations to read.");
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each element, shown in the Project Map (ModifySections changes it).");

            OutTexts(
                "IDs",
                "Reference ID of each element, shown in its marker and in the Project Map (ModifySections changes it).");

            OutTexts(
                "Details",
                "The settings of each element, as a JSON object (SetSectionsAndElevationsDetails takes it back).");

            OutTexts(
                "ReadOnlyDetails",
                "The read-only state of each element, as a JSON object: the distant area shadow and fill " +
                "settings Archicad copies from the other settings, the current plan connectivity, whether its " +
                "window is opened, whether it is active, a source marker or in drawing mode, and its marker " +
                "elements (beginMarkerElementId, middleMarkerElementId, endMarkerElementId).");

            OutErrorMessages(
                "Error message of each element (empty when it was read).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var names = new List<string>();
            var ids = new List<string>();
            var details = new List<string>();
            var readOnlyDetails = new List<string>();
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item) ||
                    !(item is JObject itemObject))
                {
                    names.Add(null);
                    ids.Add(null);
                    details.Add(null);
                    readOnlyDetails.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                names.Add(itemObject["name"]?.ToString());
                ids.Add(itemObject["id"]?.ToString());

                SectionElevationSettingsHelp.Split(
                    itemObject,
                    out var settable,
                    out var readOnly);
                details.Add(SectionElevationSettingsHelp.ToText(settable));
                readOnlyDetails.Add(SectionElevationSettingsHelp.ToText(readOnly));
                errors.Add("");
            }

            da.SetDataList(0, names);
            da.SetDataList(1, ids);
            da.SetDataList(2, details);
            da.SetDataList(3, readOnlyDetails);
            da.SetDataList(4, errors);
        }

        // Its own icon is in IconSource/GetSectionsAndElevationsDetails.svg; until
        // its PNG is rendered, the one of CreateSections, the other section component.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSections;

        public override Guid ComponentGuid =>
            new Guid("3b9e6f21-84c7-4d0a-9e52-c7a1f4d86b03");
    }
}
