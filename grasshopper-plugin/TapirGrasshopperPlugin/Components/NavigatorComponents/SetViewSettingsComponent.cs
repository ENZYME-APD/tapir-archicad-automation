using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

// ReSharper disable All

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class SetViewSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "SetViewSettings";

        public SetViewSettingsComponent()
            : base(
                "SetViewSettings",
                "Sets the view settings of navigator items.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifiers of the navigator items.");

            InText(
                nameof(ViewSettings.ModelViewOptions),
                "The name of the model view options. " +
                "If empty, the view has custom model view options.");

            InText(
                nameof(ViewSettings.LayerCombination),
                "The name of the layer combination. " +
                "If empty, the view has custom layer combination.");

            InText(
                nameof(ViewSettings.DimensionStyle),
                "The name of the dimension style. " +
                "If empty, the view has custom dimension style.");

            InText(
                nameof(ViewSettings.PenSetName),
                "The name of the pen set. " +
                "If empty, the view has custom pen set.");

            InText(
                nameof(ViewSettings.GraphicOverrideCombination),
                "The name of the graphic override combination. " +
                "If empty, the view has custom graphic override combination.");

            // Appended after the original inputs, so saved definitions keep their wiring.
            InText(
                nameof(ViewSettings.StructureDisplay),
                "Structure display mode: EntireStructure, CoreOnly, WithoutFinishes or StructureOnly. " +
                "If empty, it is left unchanged.");

            InText(
                nameof(ViewSettings.RenovationFilterGuid),
                "Identifier of the renovation filter to apply. If empty, it is left unchanged.");

            InText(
                nameof(ViewSettings.D3StyleName),
                "The name of the 3D style. If empty, it is left unchanged.");

            InText(
                nameof(ViewSettings.RenderingSceneName),
                "The name of the rendering scene. If empty, it is left unchanged.");

            inManager.AddBooleanParameter(
                nameof(ViewSettings.UsePhotoRendering),
                nameof(ViewSettings.UsePhotoRendering),
                "Use photo rendering for the view. If empty, it is left unchanged.",
                GH_ParamAccess.item);

            SetOptionality(
                new[]
                {
                    1,
                    2,
                    3,
                    4,
                    5,
                    6,
                    7,
                    8,
                    9,
                    10
                });
        }

        protected override void AddOutputs()
        {
            OutText(
                "Json" + nameof(ViewSettings),
                "JSON object of the retrieved view settings.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject items))
            {
                return;
            }

            var options = da.GetOptional(
                1,
                "");
            var layer = da.GetOptional(
                2,
                "");
            var style = da.GetOptional(
                3,
                "");
            var pen = da.GetOptional(
                4,
                "");
            var graphic = da.GetOptional(
                5,
                "");

            var structureDisplay = da.GetOptional<string>(
                6,
                null);
            var renovationFilterText = da.GetOptional<string>(
                7,
                null);
            var d3StyleName = da.GetOptional<string>(
                8,
                null);
            var renderingSceneName = da.GetOptional<string>(
                9,
                null);
            var usePhotoRendering = false;
            var hasUsePhotoRendering = da.GetData(
                10,
                ref usePhotoRendering);

            RenovationFilterGuid renovationFilterGuid = null;
            if (!string.IsNullOrEmpty(renovationFilterText))
            {
                renovationFilterGuid = RenovationFilterGuid.FromString(renovationFilterText);
                if (renovationFilterGuid == null)
                {
                    this.AddError("Invalid identifier in the RenovationFilterGuid input.");
                    return;
                }
            }

            var settings = new List<SetViewSettingsObject>();

            for (int i = 0; i < items.GuidWrappers.Count; i++)
            {
                settings.Add(
                    new SetViewSettingsObject
                    {
                        NavigatorGuid = items.GuidWrappers[i].Id,
                        ViewSettings = new ViewSettings
                        {
                            ModelViewOptions = options,
                            LayerCombination = layer,
                            DimensionStyle = style,
                            PenSetName = pen,
                            GraphicOverrideCombination = graphic,
                            StructureDisplay = string.IsNullOrEmpty(structureDisplay)
                                ? null
                                : structureDisplay,
                            RenovationFilterGuid = renovationFilterGuid,
                            D3StyleName = string.IsNullOrEmpty(d3StyleName)
                                ? null
                                : d3StyleName,
                            RenderingSceneName = string.IsNullOrEmpty(renderingSceneName)
                                ? null
                                : renderingSceneName,
                            UsePhotoRendering = hasUsePhotoRendering
                                ? usePhotoRendering
                                : (bool?)null
                        }
                    });
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new { navigatorItemIdsWithViewSettings = settings },
                    ToAddOn,
                    ExecutionResultsResponse.Deserialize,
                    out ExecutionResultsResponse response))
            {
                return;
            }

            da.SetDataList(
                0,
                response.ExecutionResults.Select(x => x.Message()));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("1bed0140-6c27-4263-9c19-5ae234e68cd9");
    }
}