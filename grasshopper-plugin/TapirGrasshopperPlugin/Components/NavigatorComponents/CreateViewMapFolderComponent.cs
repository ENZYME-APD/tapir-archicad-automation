using Grasshopper.Kernel;
using Newtonsoft.Json;
using System;
using TapirGrasshopperPlugin.Data;

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
        public static string Doc =>
            "Creates a view folder item at the given position in the navigator tree.";

        public override string CommandName => "CreateViewMapFolder";

        public CreateViewMapFolder()
            : base(
                "CreateViewMapFolder",
                "CreateViewMapFolder",
                Doc,
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            AddText(
                "Name",
                "Name of the new folder.");

            AddGeneric(
                "ParentNavigatorItemId",
                "The newly created folder will be placed under this parent item. " +
                "If this parameter is not given the folder will be created as the first item in the View Map list.");

            AddGeneric(
                "PreviousNavigatorItemId",
                "The newly created folder will be placed after this sibling item. " +
                "If this parameter is not given the folder will be created as the first item under the parent.");

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
            IGH_DataAccess da)
        {
            var name = "";
            if (!da.GetData(
                    0,
                    ref name))
            {
                return;
            }

            var parentNavigatorItemId = NavigatorIdItemObj.Create(
                da,
                1);
            var previousNavigatorItemId = NavigatorIdItemObj.Create(
                da,
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

            if (!GetConvertedResponse(
                    CommandName,
                    input,
                    out CreateViewMapFolderOutput response))
            {
                return;
            }

            var createdId = new NavigatorIdItemObj()
            {
                Id = response.CreatedFolderNavigatorItemId
            };

            da.SetData(
                0,
                createdId);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateViewMapFolder;

        public override Guid ComponentGuid =>
            new Guid("4de02e9a-55c3-4d23-9f96-bb5f73d50f0e");
    }
}