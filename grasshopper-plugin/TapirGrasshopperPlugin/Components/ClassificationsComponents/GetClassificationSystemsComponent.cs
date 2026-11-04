using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ClassificationsComponents
{
    // Sends the Tapir version of GetAllClassificationSystems (same output as
    // the official command, which AllClassifications and the value list use).
    public class GetClassificationSystemsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAllClassificationSystems";

        public GetClassificationSystemsComponent()
            : base(
                "GetClassificationSystems",
                "Get the classification systems of the project with their guid, name, description, source, " +
                "version and date.",
                GroupNames.Classifications)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ClassificationSystemGuids",
                "Identifier of each classification system.");

            OutTexts(
                "Names",
                "Display name of each classification system.");

            OutTexts(
                "Descriptions",
                "Description of each classification system.");

            OutTexts(
                "Sources",
                "Source of each classification system (e.g. URL to a classification system standard).");

            OutTexts(
                "Versions",
                "Version of each classification system.");

            OutTexts(
                "Dates",
                "Release date of the current version of each classification system (YYYY-MM-DD).");
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

            var systemGuids = new List<ClassificationSystemGuidWrapper>();
            var names = new List<string>();
            var descriptions = new List<string>();
            var sources = new List<string>();
            var versions = new List<string>();
            var dates = new List<string>();

            foreach (var system in JsonOutputHelp.Items(
                         response,
                         "classificationSystems"))
            {
                var guid = JsonOutputHelp.WrappedGuidOf(
                    system,
                    "classificationSystemId");
                systemGuids.Add(
                    guid == null
                        ? null
                        : new ClassificationSystemGuidWrapper
                        {
                            ClassificationSystemId =
                                new ClassificationGuid { Guid = guid }
                        });
                names.Add(system["name"]?.ToString());
                descriptions.Add(system["description"]?.ToString());
                sources.Add(system["source"]?.ToString());
                versions.Add(system["version"]?.ToString());
                dates.Add(system["date"]?.ToString());
            }

            da.SetDataList(0, systemGuids);
            da.SetDataList(1, names);
            da.SetDataList(2, descriptions);
            da.SetDataList(3, sources);
            da.SetDataList(4, versions);
            da.SetDataList(5, dates);
        }

        // Shares the icon of the classification systems value list.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ClassificationSystemsValueList;

        public override Guid ComponentGuid =>
            new Guid("134e555d-e12f-4ba8-a775-f18a1f3298ab");
    }
}
