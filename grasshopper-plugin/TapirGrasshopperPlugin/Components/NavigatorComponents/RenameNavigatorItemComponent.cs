using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class RenameNavigatorItemInput
    {
        [JsonProperty("navigatorItemId")]
        public NavigatorIdObj NavigatorItemId;

        [JsonProperty(
            "newName",
            NullValueHandling = NullValueHandling.Ignore)]
        public string NewName;

        [JsonProperty(
            "newId",
            NullValueHandling = NullValueHandling.Ignore)]
        public string NewId;
    }

    public class RenameNavigatorItem : ArchicadExecutorComponent
    {
        public RenameNavigatorItem()
            : base(
                "RenameNavigatorItem",
                "RenameNavigatorItem",
                "Renames a navigator item.",
                "Navigator")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "NavigatorItemId",
                "NavigatorItemId",
                "Identifier of a navigator item to rename.",
                GH_ParamAccess.item);
            pManager.AddTextParameter(
                "New Name",
                "NewName",
                "New name for the navigator item.",
                GH_ParamAccess.item,
                @default: "");
            pManager.AddTextParameter(
                "New Id",
                "NewId",
                "New id for the navigator item.",
                GH_ParamAccess.item,
                @default: "");
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var navigatorItemId = NavigatorIdItemObj.Create(
                DA,
                0);
            if (navigatorItemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input NavigatorItemId failed to collect data.");
                return;
            }

            var newName = "";
            if (!DA.GetData(
                    1,
                    ref newName))
            {
                return;
            }

            var newId = "";
            if (!DA.GetData(
                    2,
                    ref newId))
            {
                return;
            }

            var input = new RenameNavigatorItemInput()
            {
                NavigatorItemId = navigatorItemId.Id,
                NewName = String.IsNullOrEmpty(newName) ? null : newName,
                NewId = String.IsNullOrEmpty(newId) ? null : newId
            };
            var inputObj = JObject.FromObject(input);
            var response = SendArchicadCommand(
                "RenameNavigatorItem",
                inputObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}