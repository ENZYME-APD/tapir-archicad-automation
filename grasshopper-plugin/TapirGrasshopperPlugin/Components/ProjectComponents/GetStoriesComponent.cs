using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.ResponseTypes.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetStoriesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetStories";

        public GetStoriesComponent()
            : base(
                "Stories",
                "Retrieves information about the story structure of the currently loaded project.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts("StoryNames");
            OutNumbers("StoryElevations");
            OutNumbers("StoryHeights");
            OutBooleans("ShowOnSections");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetConvertedResponse(
                    CommandName,
                    out StoriesData response))
            {
                return;
            }

            var names = new List<string>();
            var elevations = new List<double>();
            var heights = new List<double>();
            var showOnSections = new List<bool>();

            for (var i = 0; i < response.Stories.Count; ++i)
            {
                names.Add(response.Stories[i].Name);
                elevations.Add(response.Stories[i].Level);
                if (i < response.Stories.Count - 1)
                {
                    heights.Add(
                        response.Stories[i + 1].Level -
                        response.Stories[i].Level);
                }
                else if (heights.Count > 0)
                {
                    heights.Add(heights.Last());
                }
                else
                {
                    heights.Add(10.0);
                }

                showOnSections.Add(response.Stories[i].DispOnSections);
            }

            da.SetDataList(
                0,
                names);

            da.SetDataList(
                1,
                elevations);

            da.SetDataList(
                2,
                heights);

            da.SetDataList(
                3,
                showOnSections);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetStories;

        public override Guid ComponentGuid =>
            new Guid("c06545cd-ec1b-439a-9c70-0cef732cca9a");
    }
}