using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class FolderParameters
    {
        [JsonProperty("name")]
        public string Name;
    }

    public class CreateViewMapFolderInput
    {
        [JsonProperty("folderParameters")]
        public FolderParameters FolderParameters;

        [JsonProperty(
            "parentNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj ParentNavigatorItemId;

        [JsonProperty(
            "previousNavigatorItemId",
            NullValueHandling = NullValueHandling.Ignore)]
        public NavigatorIdObj PreviousNavigatorItemId;
    }

    public class CreateViewMapFolderOutput
    {
        [JsonProperty("createdFolderNavigatorItemId")]
        public NavigatorIdObj CreatedFolderNavigatorItemId;
    }

    public class CreateViewMapFolder : ArchicadExecutorComponent
    {
        public CreateViewMapFolder()
            : base(
                "CreateViewMapFolder",
                "CreateViewMapFolder",
                "Creates a view folder item at the given position in the navigator tree.",
                "Navigator")
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Name",
                "Name",
                "Name of the new folder.",
                GH_ParamAccess.item);
            pManager.AddGenericParameter(
                "ParentNavigatorItemId",
                "ParentNavigatorItemId",
                "The newly created folder will be placed under this parent item. If this parameter is not given the folder will be created as the first item in the View Map list.",
                GH_ParamAccess.item);
            pManager.AddGenericParameter(
                "PreviousNavigatorItemId",
                "PreviousNavigatorItemId",
                "The newly created folder will be placed after this sibling item. If this parameter is not given the folder will be created as the first item under the parent.",
                GH_ParamAccess.item);

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "CreatedNavigatorItemId",
                "CreatedNavigatorItemId",
                "The ID of the new navigator item in view map.",
                GH_ParamAccess.item);
        }

        protected override void Solve(
            IGH_DataAccess DA)
        {
            var name = "";
            if (!DA.GetData(
                    0,
                    ref name))
            {
                return;
            }

            var parentNavigatorItemId = NavigatorIdItemObj.Create(
                DA,
                1);
            var previousNavigatorItemId = NavigatorIdItemObj.Create(
                DA,
                2);

            var input = new CreateViewMapFolderInput()
            {
                FolderParameters = new FolderParameters() { Name = name },
                ParentNavigatorItemId =
                    parentNavigatorItemId == null
                        ? null
                        : parentNavigatorItemId.Id,
                PreviousNavigatorItemId = previousNavigatorItemId == null
                    ? null
                    : previousNavigatorItemId.Id
            };
            var inputObj = JObject.FromObject(input);
            var response = SendArchicadCommand(
                "CreateViewMapFolder",
                inputObj);
            if (!response.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    response.GetErrorMessage());
                return;
            }

            var createdFolderNavigatorItemId = new NavigatorIdItemObj()
            {
                Id = response.Result.ToObject<CreateViewMapFolderOutput>()
                    .CreatedFolderNavigatorItemId
            };
            DA.SetData(
                0,
                createdFolderNavigatorItemId);
        }

        protected override System.Drawing.Bitmap Icon =>
            TapirGrasshopperPlugin.Properties.Resources.CreateViewMapFolder;

        public override Guid ComponentGuid =>
            new Guid("4de02e9a-55c3-4d23-9f96-bb5f73d50f0e");
    }
}