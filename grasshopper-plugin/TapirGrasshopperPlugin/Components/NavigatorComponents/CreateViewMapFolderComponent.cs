using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateViewMapFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateViewMapFolder";

        public CreateViewMapFolderComponent()
            : base(
                "CreateViewMapFolderComponent",
                "Creates a view folder item at the given position in the navigator tree.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText("FolderName");

            InGeneric(
                "ParentNavigatorItemId",
                "The newly created folder will be placed under this parent item. " +
                "If this parameter is not given the folder will be created as the first item in the View Map list.");

            InGeneric(
                "PreviousNavigatorItemId",
                "The newly created folder will be placed after this sibling item. " +
                "If this parameter is not given the folder will be created as the first item under the parent.");

            Params.Input[1].Optional = true;
            Params.Input[2].Optional = true;
        }

        protected override void AddOutputs()
        {
            OutGeneric(
                "CreatedNavigatorItemId",
                "The ID of the new navigator item in view map.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetItem(
                    0,
                    out string name))
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

            if (!TryGetConvertedValues(
                    CommandName,
                    input,
                    SendToArchicad,
                    JHelp.Deserialize<CreateViewMapFolderOutput>,
                    out CreateViewMapFolderOutput response))
            {
                return;
            }

            var createdId = new NavigatorIdItemObj
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