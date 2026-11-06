using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class RenameNavigatorItem : ArchicadExecutorComponent
    {
        public override string CommandName => "RenameNavigatorItem";

        public RenameNavigatorItem()
            : base(
                "RenameNavigatorItem",
                "Renames a navigator item.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InGeneric(
                "NavigatorItemId",
                "Identifier of navigator items to rename.");

            InTextWithDefault(
                "NewName",
                "New name for the navigator item.",
                defaultValue: "");

            InTextWithDefault(
                "NewId",
                "New id for the navigator item.",
                defaultValue: "");

            InBoolean(
                "CustomName",
                "Set to false to drop the custom name of the item, so that it follows the name of its " +
                "Project Map source again (NewName must be empty then). Optional, true by default.",
                true);

            SetOptionality(3);
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

            if (!da.TryGet(
                    1,
                    out string newName))
            {
                return;
            }

            if (!da.TryGet(
                    2,
                    out string newId))
            {
                return;
            }

            var customName = true;
            da.GetData(
                3,
                ref customName);

            if (customName)
            {
                SetCadValues(
                    CommandName,
                    new { navigatorItemId, newName, newId },
                    ToArchicad);
                return;
            }

            // Only the Tapir command can drop the custom name; it fails when
            // a new name is given as well.
            var parameters = new JObject
            {
                ["navigatorItemId"] = JObject.FromObject(navigatorItemId),
                ["customName"] = false
            };
            if (!string.IsNullOrEmpty(newName))
            {
                parameters["newName"] = newName;
            }
            if (!string.IsNullOrEmpty(newId))
            {
                parameters["newId"] = newId;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            if ((bool?)response?["success"] != true)
            {
                this.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    "Failed to rename the navigator item.");
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}