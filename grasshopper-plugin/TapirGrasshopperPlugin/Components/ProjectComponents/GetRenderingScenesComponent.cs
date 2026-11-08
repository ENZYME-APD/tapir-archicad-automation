using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetRenderingScenesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetRenderingScenes";

        public GetRenderingScenesComponent()
            : base(
                "GetRenderingScenes",
                "Get the names of the rendering scenes of the project, in the order of the " +
                "Scene list of the PhotoRendering Settings palette. Archicad does not tell " +
                "which one is current.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "SceneNames",
                "Name of each rendering scene, as listed in the PhotoRendering Settings " +
                "palette. Feed one of them to SetCurrentRenderingScene or PhotoRender.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var sceneNames = new List<string>();

            foreach (var item in JsonOutputHelp.Items(response, "sceneNames"))
            {
                sceneNames.Add(item?.ToString() ?? "");
            }

            da.SetDataList(0, sceneNames);
        }

        // Shares the icon of the publisher set names until the rendering
        // components get their own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.PublisherSetNames;

        public override Guid ComponentGuid =>
            new Guid("6c1f0b2e-3d4a-4f58-9a7b-2e8c5d1f4a90");
    }
}
