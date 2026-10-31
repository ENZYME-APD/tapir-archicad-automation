using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class PhotoRenderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "PhotoRender";

        public PhotoRenderComponent()
            : base(
                "PhotoRender",
                "Render the 3D model with a rendering scene into an image file (png, jpg, " +
                "bmp, tiff or gif), as Document > Creative Imaging > PhotoRender Projection " +
                "does. The size and quality of the image come from the scene, and the model " +
                "is rendered as the 3D window shows it, so set up the 3D window with the " +
                "view to render first.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InText(
                "ImageFilePath",
                "Absolute path of the image file to write. An existing file is overwritten.");

            InText(
                "Format",
                "Optional. The image file format: png, jpg, bmp, tiff or gif. When it is not " +
                "given, it comes from the extension of ImageFilePath (.png, .jpg or .jpeg, " +
                ".bmp, .tif or .tiff, .gif).");

            InText(
                "SceneName",
                "Optional. The rendering scene to render with, one of the names " +
                "GetRenderingScenes returns. It is made the current scene first, as " +
                "SetCurrentRenderingScene does, and stays current afterwards. When it is " +
                "not given, the current scene is used.");

            SetOptionality(new[] { 1, 2 });
        }

        protected override void AddOutputs()
        {
            OutText(
                "ImageFilePath",
                "Path of the written image file. Empty when the render failed.");

            OutText(
                nameof(ExecutionResult.Message),
                ExecutionResult.Doc);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGet(
                    0,
                    out string imageFilePath) ||
                string.IsNullOrWhiteSpace(imageFilePath))
            {
                return;
            }

            var parameters = new JObject
            {
                ["imageFilePath"] = imageFilePath
            };

            var format = da.GetOptional<string>(1, null);
            if (!string.IsNullOrWhiteSpace(format))
            {
                parameters["format"] = format.Trim();
            }

            var sceneName = da.GetOptional<string>(2, null);
            if (!string.IsNullOrWhiteSpace(sceneName))
            {
                parameters["sceneName"] = sceneName;
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

            da.SetData(
                0,
                response.Success ? imageFilePath : "");

            da.SetData(1, response.Message());
        }

        // Shares the icon of SaveCurrentWindowAsImage until the rendering
        // components get their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SaveCurrentWindowAsImage;

        public override Guid ComponentGuid =>
            new Guid("d4a82f17-6b3e-4c9d-a1f5-3e7b8c2d9f46");
    }
}
