using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetSectionElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSectionElements";

        public GetSectionElementsComponent()
            : base(
                "GetSectionElements",
                "Gets the elements drawn in the given section, elevation or interior elevation databases, " +
                "each with the owner element it was generated from. The SectionElementGuids output is the " +
                "only source of raw section element identifiers, which CreateAssociativeDimensionsOnSection " +
                "needs - every other listing component returns the owner elements instead.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "Databases",
                "Identifiers of the section, elevation or interior elevation databases. " +
                "Leave empty to use the current database. Optional.");

            SetOptionality(0);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "SectionElementGuids",
                "Identifiers of the elements drawn in the databases, accepted as SectionElementGuids by CreateAssociativeDimensionsOnSection.");

            OutGenerics(
                "OwnerElementGuids",
                "Identifier of the owner element each section element was generated from.");

            OutTexts(
                "OwnerElementTypes",
                "Type of each owner element (empty when it cannot be resolved from the section database).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var databases = DatabasesObject.Create(
                da,
                0);

            var parameters = new JObject();
            if (databases != null && databases.Databases.Count > 0)
            {
                parameters["databases"] = JArray.FromObject(databases.Databases);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var sectionElementGuids = new List<object>();
            var ownerElementGuids = new List<object>();
            var ownerElementTypes = new List<string>();

            if (response["sectionElements"] is JArray items)
            {
                foreach (var item in items)
                {
                    sectionElementGuids.Add(
                        CreateElementGuid(item?["sectionElementId"]?["guid"]?.ToString()));
                    ownerElementGuids.Add(
                        CreateElementGuid(item?["ownerElementId"]?["guid"]?.ToString()));
                    ownerElementTypes.Add(
                        item?["ownerElementType"]?.ToString() ?? "");
                }
            }

            da.SetDataList(0, sectionElementGuids);
            da.SetDataList(1, ownerElementGuids);
            da.SetDataList(2, ownerElementTypes);
        }

        private static ElementGuidWrapper CreateElementGuid(
            string guid)
        {
            return guid == null
                ? null
                : new ElementGuidWrapper
                {
                    ElementId = new ElementGuid { Guid = guid }
                };
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSectionElements;

        public override Guid ComponentGuid =>
            new Guid("7cf1b4a8-2e35-4d90-95a7-1b0d6e3f8a54");
    }
}
