using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetStoriesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetStories";

        public SetStoriesComponent()
            : base(
                "SetStories",
                "Sets the story structure of the currently loaded project.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts("StoryNames");
            InNumbers("StoryElevations");
            InBooleans("ShowOnSections");

            inManager.AddIntegerParameter(
                "FirstStoryIndex",
                "FirstStoryIndex",
                "Index of the first (lowest) story of the list, e.g. -1 to start with a basement. " +
                "Optional; by default the stories are matched to the existing ones from the bottom up.",
                GH_ParamAccess.item);

            SetOptionality(3);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<double> elevations))
            {
                return;
            }

            if (names.Count != elevations.Count)
            {
                this.AddError("StoryName to StoryElevation count mismatch!");
                return;
            }

            if (!da.TryGetList(
                    2,
                    out List<bool> showOnSections))
            {
                return;
            }

            if (names.Count != showOnSections.Count &&
                showOnSections.Count != 1)
            {
                this.AddError("StoryName to ShowOnSections count mismatch!");
                return;
            }

            var firstStoryIndex = 0;
            var hasFirstStoryIndex = da.GetData(
                3,
                ref firstStoryIndex);

            var input = new StoriesData { Stories = new List<StoryData>() };

            for (var i = 0; i < names.Count; ++i)
            {
                input.Stories.Add(
                    new StoryData()
                    {
                        Name = names[i],
                        Level = elevations[i],
                        DispOnSections =
                            showOnSections[i % showOnSections.Count],
                        Index = i == 0 && hasFirstStoryIndex
                            ? firstStoryIndex
                            : (int?)null
                    });
            }

            SetCadValues(
                CommandName,
                input,
                ToAddOn);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetStories;

        public override Guid ComponentGuid =>
            new Guid("95ce8c3c-4525-4f60-bc1b-871beddc6f38");
    }
}