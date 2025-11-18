using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class StoryData
    {
        [JsonProperty("name")]
        public string Name;

        [JsonProperty("level")]
        public double Level;

        [JsonProperty("dispOnSections")]
        public bool DispOnSections;
    }

    public class StoriesData
    {
        [JsonProperty("stories")]
        public List<StoryData> Stories;
    }

    public class GetStoriesComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Retrieves information about the story structure of the currently loaded project.";

        public override string CommandName => "GetStories";

        public GetStoriesComponent()
            : base(
                "Stories",
                "Stories",
                Doc,
                GroupNames.Project)
        {
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Story Name",
                "StoryName",
                "Name of the story.",
                GH_ParamAccess.list);
            pManager.AddNumberParameter(
                "Story Elevation",
                "StoryElevation",
                "Elevation of the story.",
                GH_ParamAccess.list);
            pManager.AddNumberParameter(
                "Story Height",
                "StoryHeight",
                "Height of the story.",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "Show on Sections",
                "ShowOnSections",
                "Show Story on Sections.",
                GH_ParamAccess.list);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!GetConvertedResponse(
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