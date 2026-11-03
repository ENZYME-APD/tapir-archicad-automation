using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetGhostStorySettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGhostStorySettings";

        public GetGhostStorySettingsComponent()
            : base(
                "GetGhostStorySettings",
                "Get the Trace & Reference ghost story settings of the floor plan: whether it is on, " +
                "whether one story, the story above or the one below is shown, which story, its color " +
                "and the element types shown in it.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "On",
                "True when the ghost story is shown on the floor plan.");

            OutBoolean(
                "ShowOne",
                "True when the story at StoryIndex is the ghost story.");

            OutBoolean(
                "ShowAbove",
                "True when the story above the current one is the ghost story.");

            OutBoolean(
                "ShowBelow",
                "True when the story below the current one is the ghost story.");

            OutInteger(
                "StoryIndex",
                "Index of the ghost story, as GetStories reports it. Meaningful when ShowOne is true.");

            OutColorItem(
                "GhostColor",
                "Color the ghost story is drawn in.");

            OutTexts(
                "Filter",
                "Element types shown in the ghost story.");
        }

        private void OutColorItem(
            string name,
            string description)
        {
            outManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorItem"),
                GH_ParamAccess.item);
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

            da.SetData(0, response.Value<bool?>("on") ?? false);
            da.SetData(1, response.Value<bool?>("showOne") ?? false);
            da.SetData(2, response.Value<bool?>("showAbove") ?? false);
            da.SetData(3, response.Value<bool?>("showBelow") ?? false);
            da.SetData(4, response.Value<int?>("storyIndex") ?? 0);
            da.SetData(5, JsonOutputHelp.ColorOf(response["ghostColor"]));

            var filter = response["filter"] is JArray types
                ? types.Select(t => t.ToString()).ToList()
                : new List<string>();
            da.SetDataList(6, filter);
        }

        // Shares the icon of GetViewSettings until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("3f8c1a6e-7b2d-4e0a-9c5f-2a1d8e6b4c71");
    }
}
