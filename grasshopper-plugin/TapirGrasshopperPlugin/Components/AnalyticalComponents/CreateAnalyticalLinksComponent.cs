using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class CreateAnalyticalLinksComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateAnalyticalLinks";

        public CreateAnalyticalLinksComponent()
            : base(
                "CreateAnalyticalLinks",
                "Creates analytical links from their settings and connected member parts, as " +
                "GetAnalyticalLinks returns them, in one undo step. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Links",
                "Each link to create as JSON, as the Links output of GetAnalyticalLinks gives it: " +
                "{\"linkSettings\": ..., \"startElementPartRef\": ..., \"endElementPartRef\": ...}. Each is " +
                "created from the defaults of the Analytical Link tool.");
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each created link.");

            OutErrorMessages(
                "Error message of each link (empty when it was created).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(0, out List<string> texts) ||
                !AnalyticalJsonHelp.TryParseObjects(this, texts, "Links", out List<JObject> items))
            {
                return;
            }

            var links = new JArray();
            foreach (var item in items)
            {
                links.Add(AnalyticalJsonHelp.Unwrap(item, "link"));
            }

            SetCadValuesWithCreatedIds<ElementGuid>(
                CommandName,
                new JObject { ["links"] = links },
                ToAddOn,
                da,
                "elements",
                "elementId");
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("cd153cf9-b671-4342-8343-7a3d7251338f");
    }
}
