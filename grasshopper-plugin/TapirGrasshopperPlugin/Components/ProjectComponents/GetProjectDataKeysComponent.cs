using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // Project data: text (for example JSON) that scripts and Grasshopper
    // definitions save in the project under a name.
    internal static class ProjectDataHelp
    {
        public const string NamesDescription =
            "Names of the project data. Prefix them with the name of your tool (for example 'my-tool/settings') " +
            "so different scripts do not overwrite each other's data.";

        // The names input (index 0) as the "names" array of the commands.
        public static bool TryGetNames(
            IGH_DataAccess da,
            out JObject parameters)
        {
            parameters = null;
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return false;
            }

            parameters = new JObject { ["names"] = JArray.FromObject(names) };
            return true;
        }
    }

    public class GetProjectDataKeysComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProjectDataKeys";

        public GetProjectDataKeysComponent()
            : base(
                "GetProjectDataKeys",
                "Lists the project data stored by Tapir in the project: the names, the identifiers of the add-on " +
                "objects that store them, whether they are kept on this client only and the Teamwork user who has " +
                "reserved them. Project data is text (for example JSON) saved in the project file by scripts and " +
                "Grasshopper definitions, such as their settings or the mapping of elements to an external system.",
                GroupNames.Project)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "Name of each project data.");

            OutTexts(
                "Guids",
                "Identifier of the add-on object that stores each project data.");

            OutBooleans(
                "ClientOnly",
                "True when the project data is kept on this client only and is not shared in Teamwork.");

            OutIntegers(
                "TeamworkOwnerUserIds",
                "Teamwork user id of the user who has reserved the project data (null when nobody has reserved it " +
                "or the project is not a Teamwork project).");

            OutTexts(
                "TeamworkOwnerUserNames",
                "Name of the user who has reserved the project data (null when nobody has reserved it).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out var response))
            {
                return;
            }

            var names = new List<string>();
            var guids = new List<string>();
            var clientOnly = new List<bool>();
            var ownerIds = new List<int?>();
            var ownerNames = new List<string>();
            foreach (var key in JsonOutputHelp.Items(
                         response,
                         "keys"))
            {
                names.Add(key["name"]?.ToString());
                guids.Add(key["guid"]?.ToString());
                clientOnly.Add(key.Value<bool?>("clientOnly") ?? false);

                var owner = key["teamworkOwner"];
                ownerIds.Add(owner?.Value<int?>("userId"));
                ownerNames.Add(owner?["userName"]?.ToString());
            }

            da.SetDataList(0, names);
            da.SetDataList(1, guids);
            da.SetDataList(2, clientOnly);
            da.SetDataList(3, ownerIds);
            da.SetDataList(4, ownerNames);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProjectDataKeys;

        public override Guid ComponentGuid =>
            new Guid("206eb17c-f4a0-4bc2-b9a8-a2e24c201e12");
    }
}
