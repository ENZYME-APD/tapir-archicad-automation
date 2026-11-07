using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetGridElementDefaultsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetGridElementDefaults";

        public GetGridElementDefaultsComponent()
            : base(
                "GetGridElementDefaults",
                "Get the Grid Tool default that CreateGridElements starts from: its library part, line length, " +
                "story, layer and GDL parameters. Available from Archicad 26.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "LibraryPartName",
                "Name of the library part the Grid Tool places.");

            OutNumberItem(
                "Length",
                "Length of the grid line.");

            OutInteger(
                "FloorIndex",
                "Home story index.");

            OutInteger(
                "LayerIndex",
                "Layer attribute index.");

            OutBoolean(
                "UseXYFixSize",
                "Whether the object's xRatio and yRatio hold its sizes themselves.");

            OutText(
                "Parameters",
                "GDL parameters of the default as a JSON array (name, displayName, type and value).");

            OutText(
                "ParameterSettings",
                "The settable GDL parameters of the default as AdditionalSettings for CreateGridElements " +
                "(numbers, booleans and strings, without arrays, titles and AC_Length).");
        }

        private void OutNumberItem(
            string name,
            string description)
        {
            outManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item);
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

            da.SetData(0, response.Value<string>("libraryPartName"));
            da.SetData(1, response.Value<double?>("length") ?? 0.0);
            da.SetData(2, response.Value<int?>("floorIndex") ?? 0);
            da.SetData(3, response.Value<int?>("layerIndex") ?? 0);
            da.SetData(4, response.Value<bool?>("useXYFixSize") ?? false);
            da.SetData(5, GetGridElementsComponent.ParametersOf(response));
            da.SetData(6, GetGridElementsComponent.ParameterSettingsOf(response));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetGridElementDefaults;

        public override Guid ComponentGuid =>
            new Guid("af26f15b-281b-46f9-9531-5dcc49afd104");
    }
}
