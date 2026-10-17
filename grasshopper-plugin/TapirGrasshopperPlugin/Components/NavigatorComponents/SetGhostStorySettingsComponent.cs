using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetGhostStorySettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetGhostStorySettings";

        public SetGhostStorySettingsComponent()
            : base(
                "SetGhostStorySettings",
                "Set the Trace & Reference ghost story settings of the floor plan. Every input is " +
                "optional: the ones left empty keep their value. A story choice (ShowOne, ShowAbove " +
                "or ShowBelow true) replaces the current reference; without one it is kept, also " +
                "when it is another view.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            // No default values: an input with one always supplies data, which would
            // overwrite the setting instead of leaving it unchanged.
            inManager.AddBooleanParameter(
                "On",
                "On",
                "Show (true) or hide (false) the ghost story on the floor plan.",
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "ShowOne",
                "ShowOne",
                "Show the story at StoryIndex as the ghost story.",
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "ShowAbove",
                "ShowAbove",
                "Show the story above the current one as the ghost story.",
                GH_ParamAccess.item);

            inManager.AddBooleanParameter(
                "ShowBelow",
                "ShowBelow",
                "Show the story below the current one as the ghost story.",
                GH_ParamAccess.item);

            inManager.AddIntegerParameter(
                "StoryIndex",
                "StoryIndex",
                "Index of the story to show when ShowOne is true, as GetStories reports it.",
                GH_ParamAccess.item);

            InColor(
                "GhostColor",
                "Color to draw the ghost story in.");

            InTexts(
                "Filter",
                "Element types to show in the ghost story (e.g. Wall, Slab); the others are hidden.");

            SetOptionality(
                new[]
                {
                    0,
                    1,
                    2,
                    3,
                    4,
                    5,
                    6
                });
        }

        protected override void AddOutputs()
        {
            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            var on = false;
            if (da.GetData(0, ref on))
            {
                parameters["on"] = on;
            }

            var showOne = false;
            if (da.GetData(1, ref showOne))
            {
                parameters["showOne"] = showOne;
            }

            var showAbove = false;
            if (da.GetData(2, ref showAbove))
            {
                parameters["showAbove"] = showAbove;
            }

            var showBelow = false;
            if (da.GetData(3, ref showBelow))
            {
                parameters["showBelow"] = showBelow;
            }

            var storyIndex = 0;
            if (da.GetData(4, ref storyIndex))
            {
                parameters["storyIndex"] = storyIndex;
            }

            var ghostColor = Color.Empty;
            if (da.GetData(5, ref ghostColor))
            {
                parameters["ghostColor"] = new JObject
                {
                    ["red"] = ghostColor.R / 255.0,
                    ["green"] = ghostColor.G / 255.0,
                    ["blue"] = ghostColor.B / 255.0
                };
            }

            var filter = new List<string>();
            if (da.GetDataList(6, filter) && filter.Count > 0)
            {
                parameters["filter"] = new JArray(filter);
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    parameters,
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult response))
            {
                return;
            }

            if (!response.Success)
            {
                this.AddError(response.Message());
            }

            da.SetData(0, response.Message());
        }

        // Shares the icon of SetViewSettings until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("9d4b7e21-5c3a-4f86-b0e2-7f1c3a9d5e48");
    }
}
