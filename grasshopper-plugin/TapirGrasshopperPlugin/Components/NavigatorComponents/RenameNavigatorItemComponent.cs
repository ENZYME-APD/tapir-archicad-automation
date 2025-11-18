using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;

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
        public static string Doc => "Renames a navigator item.";

        public override string CommandName => "RenameNavigatorItem";

        public RenameNavigatorItem()
            : base(
                "RenameNavigatorItem",
                "RenameNavigatorItem",
                Doc,
                GroupNames.Navigator)
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
            IGH_DataAccess da)
        {
            var navigatorItemId = NavigatorIdItemObj.Create(
                da,
                0);
            if (navigatorItemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input NavigatorItemId failed to collect data.");
                return;
            }

            var newName = "";
            if (!da.GetData(
                    1,
                    ref newName))
            {
                return;
            }

            var newId = "";
            if (!da.GetData(
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

            GetResponse(
                CommandName,
                input);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.RenameNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("6be1cc4e-6645-400f-b632-1fccdae0db4d");
    }
}