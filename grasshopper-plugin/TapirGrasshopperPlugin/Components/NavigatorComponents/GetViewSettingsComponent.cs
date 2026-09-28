using Grasshopper.Kernel;
using Newtonsoft.Json;
using Rhino.Geometry;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetViewSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetViewSettings";

        public GetViewSettingsComponent()
            : base(
                nameof(ViewSettings),
                "Gets the view settings of navigator items.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "NavigatorItemIds",
                "Identifiers of the navigator items to query.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Json" + nameof(ViewSettings),
                "JSON object of the retrieved view settings.");

            OutTexts(nameof(ViewSettings.ModelViewOptions));
            OutTexts(nameof(ViewSettings.LayerCombination) + "s");
            OutTexts(nameof(ViewSettings.DimensionStyle) + "s");
            OutTexts(nameof(ViewSettings.PenSetName) + "s");
            OutTexts(nameof(ViewSettings.GraphicOverrideCombination) + "s");

            OutIntegers(
                nameof(ViewSettings.DrawingScale) + "s",
                "The drawing scale stored on each view, if enabled.");

            OutBooleans(
                nameof(ViewSettings.SaveZoom) + "s",
                "Whether the zoom box is stored in each view.");

            OutBooleans(
                nameof(ViewSettings.IgnoreSavedZoom) + "s",
                "Whether changing to each view should ignore its stored zoom.");

            OutPoints(
                "ZoomMins",
                "Minimum corner of the stored zoom box of each view (only when SaveZoom is true).");

            OutPoints(
                "ZoomMaxs",
                "Maximum corner of the stored zoom box of each view (only when SaveZoom is true).");

            OutNumberList(
                nameof(ViewSettings.Rotation) + "s",
                "View rotation angle of each view in radians.");

            OutTexts(
                nameof(ViewSettings.StructureDisplay) + "s",
                "Structure display mode of each view. One of EntireStructure, CoreOnly, WithoutFinishes, StructureOnly.");

            OutTexts(
                nameof(ViewSettings.RenovationFilterGuid) + "s",
                "GUID of the renovation filter applied to each view.");

            OutTexts(
                nameof(ViewSettings.D3StyleName) + "s",
                "Name of the 3D style of each view. Empty if not set.");

            OutTexts(
                nameof(ViewSettings.RenderingSceneName) + "s",
                "Name of the rendering scene of each view. Empty if not set.");

            OutBooleans(
                nameof(ViewSettings.UsePhotoRendering) + "s",
                "Whether photo rendering is used for each view.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out NavigatorItemsObject input))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    input,
                    ToAddOn,
                    ViewSettingsData.FromResponse,
                    out ViewSettingsData response))
            {
                return;
            }

            da.SetData(
                0,
                JsonConvert.SerializeObject(
                    response,
                    Formatting.Indented));

            // Items that are not views come back as ErrorItems - the "as"
            // cast turns them into null outputs instead of throwing.
            var settings = response.ViewSettings
                .Select(x => x as ViewSettings)
                .ToList();

            da.SetDataList(
                1,
                settings.Select(x => x?.ModelViewOptions));

            da.SetDataList(
                2,
                settings.Select(x => x?.LayerCombination));

            da.SetDataList(
                3,
                settings.Select(x => x?.DimensionStyle));

            da.SetDataList(
                4,
                settings.Select(x => x?.PenSetName));

            da.SetDataList(
                5,
                settings.Select(x => x?.GraphicOverrideCombination));

            da.SetDataList(
                6,
                settings.Select(x => x?.DrawingScale));

            da.SetDataList(
                7,
                settings.Select(x => x?.SaveZoom));

            da.SetDataList(
                8,
                settings.Select(x => x?.IgnoreSavedZoom));

            da.SetDataList(
                9,
                settings.Select(x => x?.Zoom == null
                    ? (Point3d?)null
                    : new Point3d(
                        x.Zoom.XMin,
                        x.Zoom.YMin,
                        0.0)));

            da.SetDataList(
                10,
                settings.Select(x => x?.Zoom == null
                    ? (Point3d?)null
                    : new Point3d(
                        x.Zoom.XMax,
                        x.Zoom.YMax,
                        0.0)));

            da.SetDataList(
                11,
                settings.Select(x => x?.Rotation));

            da.SetDataList(
                12,
                settings.Select(x => x?.StructureDisplay));

            da.SetDataList(
                13,
                settings.Select(x => x?.RenovationFilterGuid?.Guid));

            da.SetDataList(
                14,
                settings.Select(x => x?.D3StyleName));

            da.SetDataList(
                15,
                settings.Select(x => x?.RenderingSceneName));

            da.SetDataList(
                16,
                settings.Select(x => x?.UsePhotoRendering));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetViewSettings;

        public override Guid ComponentGuid =>
            new Guid("a0028d54-cab5-4427-9cb7-8b3ef1bb8a49");
    }
}