using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class MoveNavigatorItemInput
    {
        [JsonProperty ("navigatorItemIdToMove")]
        public NavigatorIdObj NavigatorItemIdToMove;

        [JsonProperty ("parentNavigatorItemId")]
        public NavigatorIdObj ParentNavigatorItemId;

        [JsonProperty ("previousNavigatorItemId", NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj PreviousNavigatorItemId;
    }

    public class MoveNavigatorItem : ArchicadExecutorComponent
    {
        public MoveNavigatorItem ()
          : base (
                "MoveNavigatorItem",
                "MoveNavigatorItem",
                "Moves the given navigator item under the parentNavigatorItemId in the navigator tree. If previousNavigatorItemId is not given then inserts it at the first place under the new parent. If it is given then inserts it after this navigator item.",
                "Navigator"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("NavigatorItemId", "NavigatorItemId", "Identifier of a navigator item to move.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("ParentNavigatorItemId", "ParentNavigatorItemId", "Moves the given navigator item under the parentNavigatorItemId in the navigator tree.", GH_ParamAccess.item);
            pManager.AddGenericParameter ("PreviousNavigatorItemId", "PreviousNavigatorItemId", "Moves the given navigator item after this navigator item. If it's not given then moves it at the first place under the new parent.", GH_ParamAccess.item);

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            NavigatorIdItemObj navigatorItemId = NavigatorIdItemObj.Create (DA, 0);
            if (navigatorItemId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input NavigatorItemId failed to collect data.");
                return;
            }

            NavigatorIdItemObj parentNavigatorItemId = NavigatorIdItemObj.Create (DA, 1);
            if (parentNavigatorItemId == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ParentNavigatorItemId failed to collect data.");
                return;
            }

            NavigatorIdItemObj previousNavigatorItemId = NavigatorIdItemObj.Create (DA, 2);

            MoveNavigatorItemInput input = new MoveNavigatorItemInput () {
                NavigatorItemIdToMove = navigatorItemId.Id,
                ParentNavigatorItemId = parentNavigatorItemId.Id,
                PreviousNavigatorItemId = previousNavigatorItemId == null ? null : previousNavigatorItemId.Id
            };
            JObject inputObj = JObject.FromObject (input);
            CommandResponse response = SendArchicadCommand ("MoveNavigatorItem", inputObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.MoveNavigatorItem;

        public override Guid ComponentGuid => new Guid ("fef582f6-07bd-4ffd-92cd-e41cb0447e2d");
    }
}