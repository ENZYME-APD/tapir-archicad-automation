using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Set3DFilterAndCutSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "Set3DFilterAndCutSettings";

        public Set3DFilterAndCutSettingsComponent()
            : base(
                "Set3DFilterAndCutSettings",
                "Changes the 'Filter and Cut Elements in 3D' settings of the 3D window; only the given fields and the " +
                "listed element types change. Returns the settings read back after the change. Available from " +
                "Archicad 27.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "FirstStory3D",
                "The first story shown. Optional.");

            NavigatorComponentHelp.AddOptionalInteger(
                this,
                "LastStory3D",
                "The last story shown. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "AllStories",
                "Whether all stories are shown; if false, FirstStory3D to LastStory3D. Optional.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "TrimToStoryRange",
                "Whether the elements are trimmed to the story range. Optional.");

            NavigatorComponentHelp.AddOptionalText(
                this,
                "FilterMode",
                "FilterByRules, FilterBySelection or FilterByMarqueeAndRules. Optional.");

            InTexts(
                "ElementTypes",
                "The element types to show or hide; the element types not listed keep their setting. Optional.");

            InBooleans(
                "Show",
                "Whether each element type of ElementTypes is shown (input only 1 to use the same value for all). " +
                "Required with ElementTypes.");

            NavigatorComponentHelp.AddOptionalBoolean(
                this,
                "Rebuild",
                "True to drop the partial conversion data, so the next rebuild of the 3D window is a full one. " +
                "False by default. Optional.");

            SetOptionality(new[] { 5, 6 });
        }

        protected override void AddOutputs()
        {
            Get3DFilterAndCutSettingsComponent.AddSettingsOutputs(this);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var settings = new JObject();
            NavigatorComponentHelp.CopyOptional<int>(da, 0, settings, "firstStory3D");
            NavigatorComponentHelp.CopyOptional<int>(da, 1, settings, "lastStory3D");
            NavigatorComponentHelp.CopyOptional<bool>(da, 2, settings, "allStories");
            NavigatorComponentHelp.CopyOptional<bool>(da, 3, settings, "trimToStoryRange");
            NavigatorComponentHelp.CopyOptional<string>(da, 4, settings, "filterMode");

            var elementTypes = da.OptionalList<string>(5);
            var show = da.OptionalList<bool>(6);
            if (elementTypes.Count > 0)
            {
                if (show.Count == 0)
                {
                    this.AddError("The input Show is required with ElementTypes.");
                    return;
                }

                if (!this.CheckSizes(
                        elementTypes.Count,
                        "ElementTypes",
                        ("Show", show.Count)))
                {
                    return;
                }

                var filter = new JArray();
                for (var i = 0; i < elementTypes.Count; i++)
                {
                    filter.Add(
                        new JObject
                        {
                            ["elementType"] = elementTypes[i],
                            ["show"] = show.At(i)
                        });
                }

                settings["elemTypeFilter"] = filter;
            }
            else if (show.Count > 0)
            {
                this.AddError("The input Show needs ElementTypes.");
                return;
            }

            var parameters = new JObject { ["settings"] = settings };
            NavigatorComponentHelp.CopyOptional<bool>(da, 7, parameters, "rebuild");

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            Get3DFilterAndCutSettingsComponent.SetSettingsOutputs(
                da,
                response["settings"]);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.Set3DCutPlanes;

        public override Guid ComponentGuid =>
            new Guid("3f64ed3b-d990-49c9-ad6b-f4b6e43614fe");
    }
}
