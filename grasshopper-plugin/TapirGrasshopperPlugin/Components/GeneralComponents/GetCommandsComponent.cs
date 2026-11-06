using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetCommandsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCommands";

        public GetCommandsComponent()
            : base(
                "GetCommands",
                "Gets every command of the Tapir Add-On with its command group, the Tapir version it was introduced " +
                "in and its description, so a definition can check that the installed Add-On has a command before " +
                "calling it.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutTexts(
                "Names",
                "The name of each command of the Tapir Add-On, in the order of the command reference.");

            OutTexts(
                "Groups",
                "The command group of each command, e.g. \"Application Commands\".");

            OutTexts(
                "Versions",
                "The Tapir version each command was introduced in, e.g. \"1.1.1\".");

            OutTexts(
                "Descriptions",
                "The description of each command, as in the command reference.");
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

            var names = new List<string>();
            var groups = new List<string>();
            var versions = new List<string>();
            var descriptions = new List<string>();

            foreach (var command in JsonOutputHelp.Items(response, "commands"))
            {
                names.Add(command["name"]?.ToString());
                groups.Add(command["group"]?.ToString());
                versions.Add(command["version"]?.ToString());
                descriptions.Add(command["description"]?.ToString());
            }

            da.SetDataList(0, names);
            da.SetDataList(1, groups);
            da.SetDataList(2, versions);
            da.SetDataList(3, descriptions);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.TapirVersion;

        public override Guid ComponentGuid =>
            new Guid("8a1cdd69-a6ad-4262-b3ec-90ee4e350291");
    }
}
