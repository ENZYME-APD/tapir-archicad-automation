using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class Get3DDocumentSettingsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "Get3DDocumentSettings";

        public Get3DDocumentSettingsComponent()
            : base(
                "Get3DDocumentSettings",
                "Gets the settings of a 3D Document, given by its navigator item: the element and story filter, the " +
                "3D cut, the projection, the window and zoom, the uniform cut and uncut surface attributes, the source " +
                "and the boundary contours, with the field names of API_DocumentFrom3DType.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemId",
                "The navigator item of the 3D Document, from the Project Map or the View Map.");
        }

        protected override void AddOutputs()
        {
            OutText(
                "Settings_JSON",
                "The settings of the 3D Document as a JSON object, with the field names of API_DocumentFrom3DType: " +
                Set3DDocumentSettingsComponent.SettingsKeysText + ". Change it and pass it to " +
                "Set3DDocumentSettings.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreate(
                    0,
                    out NavigatorGuid navigatorItemId))
            {
                return;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["navigatorItemId"] = JObject.FromObject(navigatorItemId) },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(0, NavigatorComponentHelp.JsonText(response["settings"]));
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetDocumentFrom3DDefaults;

        public override Guid ComponentGuid =>
            new Guid("4f1ff898-ee2a-4897-996b-4eecf2be8892");
    }
}
