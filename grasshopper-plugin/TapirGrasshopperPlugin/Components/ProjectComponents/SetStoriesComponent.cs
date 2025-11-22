using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetStoriesComponent : ArchicadExecutorComponent
    {
        public static string Doc =>
            "Sets the story structure of the currently loaded project.";

        public override string CommandName => "SetStories";

        public SetStoriesComponent()
            : base(
                "SetStories",
                "SetStories",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Story Name",
                "Name of the story.");

            InNumbers(
                "Story Elevation",
                "Elevation of the story.");

            InBooleans(
                "Show On Sections",
                "Show Story on Sections.",
                new List<bool> { true });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var names = new List<string>();
            if (!da.GetDataList(
                    0,
                    names))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input StoryName failed to collect data.");
                return;
            }

            var elevations = new List<double>();
            if (!da.GetDataList(
                    1,
                    elevations))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input StoryElevation failed to collect data.");
                return;
            }

            if (names.Count != elevations.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input StoryName and StoryElevation lists must have the same number of items.");
                return;
            }

            var showOnSections = new List<bool>();
            if (!da.GetDataList(
                    2,
                    showOnSections))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ShowOnSections failed to collect data.");
                return;
            }

            if (names.Count != showOnSections.Count &&
                showOnSections.Count != 1)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input StoryName and ShowOnSections lists must have the same number of items.");
                return;
            }

            var stories = new StoriesData { Stories = new List<StoryData>() };

            for (var i = 0; i < names.Count; ++i)
            {
                stories.Stories.Add(
                    new StoryData()
                    {
                        Name = names[i],
                        Level = elevations[i],
                        DispOnSections =
                            showOnSections[i % showOnSections.Count]
                    });
            }

            GetResponse(
                CommandName,
                stories);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetStories;

        public override Guid ComponentGuid =>
            new Guid("95ce8c3c-4525-4f60-bc1b-871beddc6f38");
    }
}