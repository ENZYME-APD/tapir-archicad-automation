using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Project;

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
            InTexts(
                "Story Name",
                "Name of the story.");

            InNumbers(
                "Story Elevation",
                "Elevation of the story.");

            InBooleans(
                "Show On Sections",
                "Show Story on Sections.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItems(
                    0,
                    out List<string> names))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<double> elevations))
            {
                return;
            }

            if (names.Count != elevations.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input StoryName and StoryElevation lists must have the same number of items.");
                return;
            }

            if (!da.TryGetItems(
                    2,
                    out List<bool> showOnSections))
            {
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

            SetArchiCadValues(
                CommandName,
                stories);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetStories;

        public override Guid ComponentGuid =>
            new Guid("95ce8c3c-4525-4f60-bc1b-871beddc6f38");
    }
}