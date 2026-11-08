using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Navigator;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetSchedulesComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetSchedules";

        public GetSchedulesComponent()
            : base(
                "GetSchedules",
                "Returns the Interactive Schedules of the Project Map: navigator item id, name, ID and the " +
                "folders they are in. Archicad's API does not give the rows of a schedule nor export it; to " +
                "export one, put it in a publisher set with a spreadsheet format in Archicad and run " +
                "PublishPublisherSet.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "NavigatorItemIds",
                "The navigator item of each schedule, in navigator order.");

            OutTexts(
                "Names",
                "The name of each schedule.");

            OutTexts(
                "IDs",
                "The ID of each schedule, shown before its name in the navigator.");

            OutTextTree(
                "FolderPaths",
                "The names of the Project Map items each schedule is in, from the top down, without the project " +
                "itself, e.g. Schedules, Element (one branch per schedule).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var navigatorItemIds = new List<NavigatorGuidWrapper>();
            var names = new List<string>();
            var ids = new List<string>();
            var folderPaths = new DataTree<string>();

            var schedules = JsonOutputHelp.Items(
                response,
                "schedules");
            for (var i = 0; i < schedules.Count; i++)
            {
                var schedule = schedules[i];
                var navigatorItemId = schedule["navigatorItemId"]?.ToObject<NavigatorGuid>();
                navigatorItemIds.Add(navigatorItemId == null ? null : new NavigatorGuidWrapper { Id = navigatorItemId });
                names.Add(schedule["name"]?.ToString());
                ids.Add(schedule["uiId"]?.ToString());

                var path = new GH_Path(i);
                folderPaths.EnsurePath(path);
                if (schedule["folderPath"] is JArray folders)
                {
                    foreach (var folder in folders)
                    {
                        folderPaths.Add(folder.ToString(), path);
                    }
                }
            }

            da.SetDataList(0, navigatorItemIds);
            da.SetDataList(1, names);
            da.SetDataList(2, ids);
            da.SetDataTree(3, folderPaths);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSchedules;

        public override Guid ComponentGuid =>
            new Guid("5d88624e-23c7-445b-a852-56d5fa5e22f4");
    }
}
