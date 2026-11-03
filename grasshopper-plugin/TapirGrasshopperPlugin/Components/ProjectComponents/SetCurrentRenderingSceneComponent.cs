using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetCurrentRenderingSceneComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetCurrentRenderingScene";

        public SetCurrentRenderingSceneComponent()
            : base(
                "SetCurrentRenderingScene",
                "Make a rendering scene the current one, as selecting it in the Scene list " +
                "of the PhotoRendering Settings palette does. It stays current afterwards " +
                "and is saved with the project.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "SceneName",
                "Name of the rendering scene to make current, one of the names " +
                "GetRenderingScenes returns.");
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
            if (!da.TryGet(
                    0,
                    out string sceneName) ||
                string.IsNullOrWhiteSpace(sceneName))
            {
                return;
            }

            var parameters = new JObject
            {
                ["sceneName"] = sceneName
            };

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

        // Shares the icon of SetViewSettings until the rendering components
        // get their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("9b7e3c51-2f6d-4a1c-8e3b-7d2a9c4f6e15");
    }
}
