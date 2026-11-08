using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Components.ElementsComponents;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetSectionsAndElevationsDetailsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetSectionsAndElevationsDetails";

        public SetSectionsAndElevationsDetailsComponent()
            : base(
                "SetSectionsAndElevationsDetails",
                "Changes the settings of Section and Elevation elements returned by GetSectionsAndElevationsDetails. " +
                "Their name and reference ID are changed with ModifySections.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the Sections or Elevations to change.");

            InTexts(
                "Details",
                SectionElevationSettingsHelp.DetailsDescription +
                " Input only 1 to use the same settings for all.");
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each element (empty when it was changed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> elementWrappers) ||
                !this.TryGetElementIds(
                    elementWrappers,
                    "ElementGuids",
                    out var elementIds))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<string> detailsTexts) ||
                !this.CheckSizes(
                    elementIds.Count,
                    "ElementGuids",
                    ("Details", detailsTexts.Count)))
            {
                return;
            }

            var items = new JArray();
            for (var i = 0; i < elementIds.Count; i++)
            {
                if (!OpeningSettingsHelp.TryParse(
                        this,
                        detailsTexts.At(i),
                        "Details",
                        SectionElevationSettingsHelp.SettableKeys,
                        out var details))
                {
                    return;
                }

                items.Add(new JObject
                {
                    ["elementId"] = elementIds[i],
                    ["details"] = details ?? new JObject()
                });
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["elementsWithDetails"] = items },
                ToAddOn,
                da);
        }

        // Its own icon is in IconSource/SetSectionsAndElevationsDetails.svg; until
        // its PNG is rendered, the one of CreateSections, the other section component.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateSections;

        public override Guid ComponentGuid =>
            new Guid("c84d2a5e-1f07-4b63-b9e8-5a0d73e2f914");
    }
}
