using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.AnalyticalComponents
{
    public class GetAnalyticalLoadsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetAnalyticalLoads";

        public GetAnalyticalLoadsComponent()
            : base(
                "GetAnalyticalLoads",
                "Retrieves the settings, the loaded member part, the custom data (the curve of an edge load, the " +
                "contour of a surface load) and the geometry of the given analytical point, edge and surface " +
                "loads. Available from Archicad 26.",
                GroupNames.Analytical)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the analytical point, edge and surface loads.");

            InGeneric(
                "AnalyticalModelGuid",
                "Identifier of the Structural Analytical Model the geometry is read from. Optional; the model " +
                "of the current view is used when it is omitted. Required in Archicad 26.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "LoadTypes",
                "Kind of each load: PointLoad, EdgeLoad or SurfaceLoad (null when it could not be got).");

            OutTexts(
                "Loads",
                "Each load as JSON without its geometry: {\"pointLoad\": ...}, or {\"edgeLoad\": ..., " +
                "\"customData\": ...} or {\"surfaceLoad\": ..., \"customData\": ...}; it can be fed to " +
                "CreateAnalyticalLoads.");

            OutTexts(
                "Geometries",
                "The geometry of each load in the Structural Analytical Model as JSON " +
                "(a coordinate, a curve or a contour).");

            OutErrorMessages(
                "Error message of each element (empty when its load was got).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();
            if (!AnalyticalJsonHelp.TryAddElements(da, 0, parameters) ||
                !AnalyticalJsonHelp.TryAddModelId(this, da, 1, parameters))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var items = AnalyticalJsonHelp.SplitErrors(
                response,
                "loads",
                out List<string> errors);

            var loadTypes = new List<string>();
            var loads = new List<string>();
            foreach (var item in items)
            {
                string loadType = null;
                JObject load = null;
                if (item is JObject itemObject)
                {
                    foreach (var pair in AnalyticalJsonHelp.LoadKinds)
                    {
                        if (itemObject[pair.Key] != null)
                        {
                            loadType = pair.Name;
                            load = AnalyticalJsonHelp.Pick(itemObject, pair.Key, "customData");
                            break;
                        }
                    }
                }
                loadTypes.Add(loadType);
                loads.Add(AnalyticalJsonHelp.TextOf(load));
            }

            da.SetDataList(0, loadTypes);
            da.SetDataList(1, loads);
            da.SetDataList(2, AnalyticalJsonHelp.TextsOf(items, "geometry"));
            da.SetDataList(3, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetAnalyticalLoadCases;

        public override Guid ComponentGuid =>
            new Guid("df51376a-c46d-4090-be5d-506eda8bc2a2");
    }
}
