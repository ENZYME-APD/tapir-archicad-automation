using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class ChangeWindowComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ChangeWindow";

        public ChangeWindowComponent()
            : base(
                "ChangeWindow",
                "Change the current (active) window to the given window. " +
                "Provide either a NavigatorItemId (activates the view with its saved settings, Archicad 27+) " +
                "or a WindowType (optionally with a StoryIndex for FloorPlan, or a DatabaseId).",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InTextWithDefault(
                "WindowType",
                "The type of the window to activate (e.g. FloorPlan, Section, 3DModel, Layout).",
                "");

            InInteger(
                "StoryIndex",
                "Story index to activate. Only used when WindowType is 'FloorPlan'.",
                0);

            InGeneric(
                "NavigatorItemId",
                "Identifier of a navigator view or viewpoint to activate. Takes precedence over WindowType.");

            InGeneric(
                "DatabaseId",
                "Identifier of the database to activate. Must belong to the requested WindowType.");

            SetOptionality(new[] { 0, 1, 2, 3 });
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var parameters = new JObject();

            if (da.TryGet(
                    2,
                    out GH_ObjectWrapper navWrapper) &&
                navWrapper?.Value != null)
            {
                var navId = GuidObject<NavigatorGuid>.CreateFromWrapper(navWrapper);
                if (navId == null)
                {
                    this.AddError("Invalid NavigatorItemId.");
                    return;
                }
                parameters["navigatorItemId"] = new JObject { ["guid"] = navId.Guid };
            }
            else
            {
                if (!da.TryGet(
                        0,
                        out string windowType) ||
                    string.IsNullOrEmpty(windowType))
                {
                    this.AddError(
                        "Provide either a NavigatorItemId or a WindowType.");
                    return;
                }
                parameters["windowType"] = windowType;

                if (windowType == "FloorPlan" &&
                    da.TryGet(
                        1,
                        out int storyIndex))
                {
                    parameters["storyIndex"] = storyIndex;
                }

                if (da.TryGet(
                        3,
                        out GH_ObjectWrapper dbWrapper) &&
                    dbWrapper?.Value != null)
                {
                    var dbId = GuidObject<DatabaseGuidObject>.CreateFromWrapper(dbWrapper);
                    if (dbId == null)
                    {
                        this.AddError("Invalid DatabaseId.");
                        return;
                    }
                    parameters["databaseId"] = new JObject { ["guid"] = dbId.Guid };
                }
            }

            TryGetCadResponse(
                CommandName,
                parameters,
                ToAddOn,
                out _);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ChangeWindow;

        public override Guid ComponentGuid =>
            new Guid("09a85ff2-d728-4b1b-9d83-3fd6228ce417");
    }
}
