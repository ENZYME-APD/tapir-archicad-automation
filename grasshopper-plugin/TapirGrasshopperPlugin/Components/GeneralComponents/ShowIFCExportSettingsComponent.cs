using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ShowIFCExportSettingsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ShowIFCExportSettings";

        public ShowIFCExportSettingsComponent()
            : base(
                "ShowIFCExportSettings",
                "Opens Archicad's IFC export settings dialog, where the user picks a view and an IFC export " +
                "translator, waits for the user and returns the chosen view and translator, or cancelled. " +
                "Archicad waits for the user, so it only runs on the Execute button. Available from Archicad 28.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemGuid",
                "The view (an item of the View Map) the dialog starts with. Optional.");

            InText(
                "TranslatorName",
                "The name of the IFC export translator the dialog starts with, as GetIFCExportTranslators lists " +
                "them. Optional.");

            SetOptionality(new[] { 0, 1 });
        }

        protected override void AddOutputs()
        {
            OutBoolean(
                "Cancelled",
                "True when the user closed the dialog with Cancel; then the other outputs are empty.");

            OutGeneric(
                "NavigatorItemGuid",
                "The view the user chose.");

            OutText(
                "ViewName",
                "The name of the chosen view, when Archicad finds it in the navigator.");

            OutText(
                "TranslatorName",
                "The name of the IFC export translator the user chose, as the TranslatorName input of " +
                "IFCFileOperation takes it.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            GH_ObjectWrapper navigatorWrapper = null;
            if (da.GetData(0, ref navigatorWrapper) &&
                navigatorWrapper != null)
            {
                var navigatorItemId = GuidObject<NavigatorGuid>.CreateFromWrapper(navigatorWrapper);
                if (navigatorItemId == null)
                {
                    this.AddError(
                        "Invalid navigator item identifier in the NavigatorItemGuid input.");
                    return;
                }

                parameters["navigatorItemId"] = new JObject { ["guid"] = navigatorItemId.Guid };
            }

            string translatorName = null;
            if (da.GetData(1, ref translatorName) &&
                !string.IsNullOrEmpty(translatorName))
            {
                parameters["translatorName"] = translatorName;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                (bool?)response["cancelled"]);

            var guid = JsonOutputHelp.GuidOf(response["navigatorItemId"]);
            da.SetData(
                1,
                guid == null
                    ? null
                    : new NavigatorGuidWrapper { NavigatorId = new NavigatorGuid { Guid = guid } });

            da.SetData(
                2,
                response["viewName"]?.ToString());

            da.SetData(
                3,
                response["translatorName"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ShowIFCExportSettings;

        public override Guid ComponentGuid =>
            new Guid("4d4e9889-300b-4107-a016-995daf4f53ad");
    }
}
