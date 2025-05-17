using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetStoriesComponent : ArchicadAccessorComponent
    {
        public SetStoriesComponent ()
          : base (
                "SetStories",
                "SetStories",
                "Sets the story sructure of the currently loaded project.",
                "Project"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddTextParameter ("Story Name", "StoryName", "Name of the story.", GH_ParamAccess.list);
            pManager.AddNumberParameter ("Story Elevation", "StoryElevation", "Elevation of the story.", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("Show on Sections", "ShowOnSections", "Show Story on Sections.", GH_ParamAccess.list, @default: new List<bool> () { true });
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            List<string> names = new List<string> ();
            if (!DA.GetDataList (0, names)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input StoryName failed to collect data.");
                return;
            }

            List<double> elevations = new List<double> ();
            if (!DA.GetDataList (1, elevations)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input StoryElevation failed to collect data.");
                return;
            }

            if (names.Count != elevations.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input StoryName and StoryElevation lists must have the same number of items.");
                return;
            }

            List<bool> showOnSections = new List<bool> ();
            if (!DA.GetDataList (2, showOnSections)) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ShowOnSections failed to collect data.");
                return;
            }

            if (names.Count != showOnSections.Count && showOnSections.Count != 1) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input StoryName and ShowOnSections lists must have the same number of items.");
                return;
            }

            StoriesData stories = new StoriesData () {
                Stories = new List<StoryData> ()
            };

            for (int i = 0; i < names.Count; ++i) {
                stories.Stories.Add (new StoryData () {
                    Name = names[i],
                    Level = elevations[i],
                    DispOnSections = showOnSections[i % showOnSections.Count]
                });
            }

            JObject storiesObj = JObject.FromObject (stories);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "SetStories", storiesObj);
            ExecutionResultObj executionResult = response.Result.ToObject<ExecutionResultObj> ();
            if (!executionResult.Success) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, executionResult.Error.Message);
            }
        }

        // protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.;

        public override Guid ComponentGuid => new Guid ("95ce8c3c-4525-4f60-bc1b-871beddc6f38");
    }
}
