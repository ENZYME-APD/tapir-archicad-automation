using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Utilities;

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
        public GetStoriesComponent()
            : base(
                "Stories",
                "Stories",
                "Retrieves information about the story sructure of the currently loaded project.",
                "Project")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
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
            IGH_DataAccess DA)
        {
            var response = SendArchicadAddOnCommand(
                "GetStories",
                null);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var stories = response.Result.ToObject<StoriesData>();
            var names = new List<string>();
            var elevations = new List<double>();
            var heights = new List<double>();
            var showOnSections = new List<bool>();
            for (var i = 0; i < stories.Stories.Count; ++i)
            {
                names.Add(stories.Stories[i].Name);
                elevations.Add(stories.Stories[i].Level);
                if (i < stories.Stories.Count - 1)
                {
                    heights.Add(
                        stories.Stories[i + 1].Level -
                        stories.Stories[i].Level);
                }
                else if (heights.Count > 0)
                {
                    heights.Add(heights.Last());
                }
                else
                {
                    heights.Add(10.0);
                }

                showOnSections.Add(stories.Stories[i].DispOnSections);
            }

            DA.SetDataList(
                0,
                names);
            DA.SetDataList(
                1,
                elevations);
            DA.SetDataList(
                2,
                heights);
            DA.SetDataList(
                3,
                showOnSections);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.GetStories;

        public override Guid ComponentGuid =>
            new Guid("c06545cd-ec1b-439a-9c70-0cef732cca9a");
    }
}