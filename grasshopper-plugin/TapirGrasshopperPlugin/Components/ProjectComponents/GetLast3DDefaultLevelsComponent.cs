using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class GetLast3DDefaultLevelsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetLast3DDefaultLevels";

        public GetLast3DDefaultLevelsComponent()
            : base(
                "GetLast3DDefaultLevels",
                "Gets the last 3D default levels of the given element types' defaults.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "ElementTypes",
                "The element types to get the last 3D default level of (e.g. Wall, Slab).");
        }

        protected override void AddOutputs()
        {
            OutNumberList(
                "Levels",
                "The last 3D default level of each element type, in the order of ElementTypes, in meters.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            AddAsSource<ElementTypeValueList>(
                document,
                0);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> elementTypes) ||
                elementTypes.Count == 0)
            {
                return;
            }

            var parameters = new JObject
            {
                ["elementTypes"] = JArray.FromObject(elementTypes)
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var levels = new List<object>();
            foreach (var level in JsonOutputHelp.Items(
                         response,
                         "levels"))
            {
                levels.Add(
                    level == null || level.Type == JTokenType.Null
                        ? (object)null
                        : (double)level);
            }

            da.SetDataList(
                0,
                levels);
        }

        // Shares the icon of GetStories until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetStories;

        public override Guid ComponentGuid =>
            new Guid("8e702b3d-1430-4bf7-89fb-f926c32c35f1");
    }
}
