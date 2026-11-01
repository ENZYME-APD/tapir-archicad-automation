using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class CloseWindowsComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CloseWindows";

        public CloseWindowsComponent()
            : base(
                "CloseWindows",
                "Closes the windows of the given databases (for example layouts, sections or details opened earlier " +
                "with ChangeWindow), with one result per window; a window Archicad cannot close gets an error message. " +
                "Requires Archicad 27 or later.",
                GroupNames.General)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "DatabaseIds",
                "The databases whose windows to close, as GetContainingDatabases or the navigator components return them.");

            InTexts(
                "WindowTypes",
                "Optional. When given, a window is only closed when its database is of this type. " +
                "Input only 1 to use the same type for all windows.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each window (empty when it was closed).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> databaseWrappers))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<string> windowTypes);

            if (windowTypes.Count > 1 &&
                windowTypes.Count != databaseWrappers.Count)
            {
                this.AddError(
                    "The WindowTypes input must hold 0, 1 or as many items as DatabaseIds.");
                return;
            }

            var windows = new JArray();
            for (var i = 0; i < databaseWrappers.Count; i++)
            {
                var databaseId = GuidObject<DatabaseGuidObject>.CreateFromWrapper(
                    databaseWrappers[i]);
                if (databaseId == null)
                {
                    this.AddError(
                        "Invalid database identifier in the DatabaseIds input.");
                    return;
                }

                var window = new JObject
                {
                    ["databaseId"] = new JObject { ["guid"] = databaseId.Guid }
                };

                var windowType = windowTypes.Count == 0
                    ? null
                    : windowTypes[windowTypes.Count == 1 ? 0 : i];
                if (!string.IsNullOrEmpty(windowType))
                {
                    window["windowType"] = windowType;
                }

                windows.Add(window);
            }

            var parameters = new JObject
            {
                ["windows"] = windows
            };

            SetCadValuesWithErrorMessages(
                CommandName,
                parameters,
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.CloseWindows;

        public override Guid ComponentGuid =>
            new Guid("ca5eed14-00c4-4d19-ae52-6541a661f5ab");
    }
}
