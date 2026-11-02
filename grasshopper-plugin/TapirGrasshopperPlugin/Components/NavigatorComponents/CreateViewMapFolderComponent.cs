using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Linq;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class CreateViewMapFolderComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateViewMapFolder";

        public CreateViewMapFolderComponent()
            : base(
                "CreateViewMapFolderComponent",
                "Creates a new folder in the View Map, at the given position in the navigator tree.",
                GroupNames.Navigator)
        {
        }

        protected override void AddInputs()
        {
            InText("FolderName");

            InGeneric(
                "ParentNavigatorItemId",
                "The newly created folder will be placed under this parent item. " +
                "If this parameter is not given the folder will be created under the root of the View Map " +
                "(or under the parent of PreviousNavigatorItemId when that is given).");

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
                    out string folderName))
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
                previousNavigatorItemId = null;
            }

            var parameters = new JObject { ["folderName"] = folderName };

            if (parentNavigatorItemId != null)
            {
                parameters["parentNavigatorItemId"] =
                    JObject.FromObject(parentNavigatorItemId);
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var createdId = response["navigatorItemId"]?.ToObject<NavigatorGuid>();

            da.SetData(
                0,
                new NavigatorGuidWrapper { Id = createdId });

            // The Tapir command creates the folder as the first item under its
            // parent; MoveNavigatorItem puts it after the given sibling.
            if (previousNavigatorItemId != null && createdId != null)
            {
                MoveAfter(
                    createdId,
                    parentNavigatorItemId,
                    previousNavigatorItemId);
            }
        }

        private void MoveAfter(
            NavigatorGuid createdId,
            NavigatorGuid parentId,
            NavigatorGuid previousId)
        {
            if (parentId == null)
            {
                if (!TryGetCadResponse(
                        "GetNavigatorParentItems",
                        new JObject
                        {
                            ["navigatorItemIds"] = new JArray
                            {
                                new JObject { ["navigatorItemId"] = JObject.FromObject(previousId) }
                            }
                        },
                        ToAddOn,
                        out JObject parentResponse))
                {
                    return;
                }

                var parentItem = JsonOutputHelp.Items(
                    parentResponse,
                    "parentItems").FirstOrDefault();

                if (parentItem == null || JsonOutputHelp.IsError(parentItem))
                {
                    this.AddError(
                        "The folder was created, but the parent of PreviousNavigatorItemId cannot be read: " +
                        JsonOutputHelp.ErrorMessage(parentItem));
                    return;
                }

                parentId = parentItem["navigatorItemId"]?.ToObject<NavigatorGuid>();
            }

            if (!TryGetConvertedCadValues(
                    "MoveNavigatorItem",
                    new JObject
                    {
                        ["navigatorItemIdToMove"] = JObject.FromObject(createdId),
                        ["parentNavigatorItemId"] = JObject.FromObject(parentId),
                        ["previousNavigatorItemId"] = JObject.FromObject(previousId)
                    },
                    ToAddOn,
                    ExecutionResult.Deserialize,
                    out ExecutionResult moveResult))
            {
                return;
            }

            if (!moveResult.Success)
            {
                this.AddError(
                    "The folder was created, but it cannot be placed after PreviousNavigatorItemId: " +
                    moveResult.Message());
            }
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CreateViewMapFolder;

        public override Guid ComponentGuid =>
            new Guid("4de02e9a-55c3-4d23-9f96-bb5f73d50f0e");
    }
}