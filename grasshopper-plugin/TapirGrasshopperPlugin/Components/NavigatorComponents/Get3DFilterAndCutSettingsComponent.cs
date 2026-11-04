using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DFilterAndCutSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DFilterAndCutSettings";

        public Get3DFilterAndCutSettingsComponent()
            : base(
                "Get3DFilterAndCutSettings",
                "Gets the 'Filter and Cut Elements in 3D' settings of the 3D window: the stories shown, the filter " +
                "mode and which element types are shown. Available from Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            Get3DFilterAndCutSettingsComponent.AddSettingsOutputs(this);
        }

        // The outputs of the settings, shared with Set3DFilterAndCutSettings.
        internal static void AddSettingsOutputs(
            Component component)
        {
            component.OutInteger(
                "FirstStory3D",
                "The first story shown.");

            component.OutInteger(
                "LastStory3D",
                "The last story shown.");

            component.OutBoolean(
                "AllStories",
                "Whether all stories are shown; if false, FirstStory3D to LastStory3D.");

            component.OutBoolean(
                "TrimToStoryRange",
                "Whether the elements are trimmed to the story range.");

            component.OutText(
                "FilterMode",
                "FilterByRules, FilterBySelection or FilterByMarqueeAndRules.");

            component.OutTexts(
                "ElementTypes",
                "The element types of the element type filter.");

            component.OutBooleans(
                "Show",
                "Whether each element type of ElementTypes is shown.");
        }

        internal static void SetSettingsOutputs(
            IGH_DataAccess da,
            JToken settings)
        {
            if (settings == null)
            {
                return;
            }

            da.SetData(0, (int?)settings["firstStory3D"]);
            da.SetData(1, (int?)settings["lastStory3D"]);
            da.SetData(2, (bool?)settings["allStories"]);
            da.SetData(3, (bool?)settings["trimToStoryRange"]);
            da.SetData(4, settings["filterMode"]?.ToString());

            var elementTypes = new List<string>();
            var show = new List<bool?>();
            if (settings["elemTypeFilter"] is JArray filter)
            {
                foreach (var item in filter)
                {
                    elementTypes.Add(item["elementType"]?.ToString());
                    show.Add((bool?)item["show"]);
                }
            }

            da.SetDataList(5, elementTypes);
            da.SetDataList(6, show);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            SetSettingsOutputs(
                da,
                response["settings"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Get3DCutPlanes;

        public override Guid ComponentGuid =>
            new Guid("471ca31f-a6ec-466f-983f-49fb1ec2a6a8");
    }
}
