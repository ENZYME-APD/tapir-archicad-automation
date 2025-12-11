using Grasshopper.Kernel;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

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

            SetOptionality(
                new[]
                {
                    1,
                    2
                });
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
            if (!da.TryGet(
                    0,
                    out string name))
            {
                return;
            }

            if (!da.TryCreate(
                    1,
                    out NavigatorGuid parentNavigatorItemId))
            {
                parentNavigatorItemId = null;
            }

            if (!da.TryCreate(
                    2,
                    out NavigatorGuid previousNavigatorItemId))
            {
                parentNavigatorItemId = null;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new
                    {
                        folderParameters = new { name },
                        parentNavigatorItemId,
                        previousNavigatorItemId
                    },
                    ToArchicad,
                    JHelp.Deserialize<ViewMapFolderOutput>,
                    out ViewMapFolderOutput response))
            {
                return;
            }

            var wrapper = new NavigatorGuidWrapper
            {
                Id = response.CreatedFolderNavigatorItemId
            };

            da.SetData(
                0,
                wrapper);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateViewMapFolder;

        public override Guid ComponentGuid =>
            new Guid("4de02e9a-55c3-4d23-9f96-bb5f73d50f0e");
    }
}