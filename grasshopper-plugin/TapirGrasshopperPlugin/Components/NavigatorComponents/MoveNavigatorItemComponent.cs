using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class MoveNavigatorItemInput
    {
        [JsonProperty("navigatorItemIdToMove")]
        public NavigatorIdObj NavigatorItemIdToMove;

        [JsonProperty("parentNavigatorItemId")]
        public NavigatorIdObj ParentNavigatorItemId;

        [JsonProperty(
            "previousNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj PreviousNavigatorItemId;
    }

    public class MoveNavigatorItem : ArchicadExecutorComponent
    {
        public static string Doc =>
            "Moves the given navigator item under the parentNavigatorItemId in the navigator tree. " +
            "If previousNavigatorItemId is not given then inserts it at the first place under the new parent. " +
            "If it is given then inserts it after this navigator item.";

        public override string CommandName => "MoveNavigatorItem";

        public MoveNavigatorItem()
            : base(
                "MoveNavigatorItem",
                "MoveNavigatorItem",
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
                "Identifier of a navigator item to move.",
                GH_ParamAccess.item);
            pManager.AddGenericParameter(
                "ParentNavigatorItemId",
                "ParentNavigatorItemId",
                "Moves the given navigator item under the parentNavigatorItemId in the navigator tree.",
                GH_ParamAccess.item);
            pManager.AddGenericParameter(
                "PreviousNavigatorItemId",
                "PreviousNavigatorItemId",
                "Moves the given navigator item after this navigator item. If it's not given then moves it at the first place under the new parent.",
                GH_ParamAccess.item);

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
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

            var parentNavigatorItemId = NavigatorIdItemObj.Create(
                da,
                1);
            if (parentNavigatorItemId == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ParentNavigatorItemId failed to collect data.");
                return;
            }

            var previousNavigatorItemId = NavigatorIdItemObj.Create(
                da,
                2);

            var input = new MoveNavigatorItemInput()
            {
                NavigatorItemIdToMove = navigatorItemId.Id,
                ParentNavigatorItemId = parentNavigatorItemId.Id,
                PreviousNavigatorItemId = previousNavigatorItemId == null
                    ? null
                    : previousNavigatorItemId.Id
            };

            GetResponse(
                CommandName,
                input);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid =>
            new Guid("fef582f6-07bd-4ffd-92cd-e41cb0447e2d");
    }
}