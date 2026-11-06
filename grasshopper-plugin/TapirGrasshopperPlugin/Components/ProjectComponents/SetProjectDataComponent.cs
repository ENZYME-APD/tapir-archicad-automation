using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class SetProjectDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "SetProjectData";

        public SetProjectDataComponent()
            : base(
                "SetProjectData",
                "Creates or overwrites project data: text (for example JSON) saved in the project under a name. " +
                "New project data can be kept on this client only, or created as unique so that in Teamwork only " +
                "one can exist with its name. Prefix the names with the name of your tool (for example " +
                "'my-tool/settings') so scripts do not overwrite each other's data.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                ProjectDataHelp.NamesDescription);

            InTexts(
                "Contents",
                "Content of each project data, for example a JSON text (input only 1 to use the same content for all).");

            InBooleans(
                "ClientOnly",
                "Creates the project data on this client only, it is not shared in Teamwork. Used only when the " +
                "project data is created (input only 1 to use the same value for all). False when empty.");

            InBooleans(
                "Unique",
                "Creates the project data as a unique add-on object: in Teamwork only one can exist with its name. " +
                "Used only when the project data is created, and cannot be combined with ClientOnly (input only 1 " +
                "to use the same value for all). False when empty.");

            SetOptionality(
                new[]
                {
                    2, 3
                });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each project data (empty when it was saved).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names) ||
                !da.TryGetList(
                    1,
                    out List<string> contents))
            {
                return;
            }

            contents = this.ExpandToCount(
                contents,
                names.Count,
                "Contents",
                "Names");
            if (contents == null)
            {
                return;
            }

            da.TryGetList(
                2,
                out List<bool> clientOnly);
            da.TryGetList(
                3,
                out List<bool> unique);

            if (clientOnly.Count > 0)
            {
                clientOnly = this.ExpandToCount(
                    clientOnly,
                    names.Count,
                    "ClientOnly",
                    "Names");
                if (clientOnly == null)
                {
                    return;
                }
            }

            if (unique.Count > 0)
            {
                unique = this.ExpandToCount(
                    unique,
                    names.Count,
                    "Unique",
                    "Names");
                if (unique == null)
                {
                    return;
                }
            }

            var values = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var value = new JObject
                {
                    ["name"] = names[i],
                    ["content"] = contents[i]
                };

                if (clientOnly.Count > 0)
                {
                    value["clientOnly"] = clientOnly[i];
                }

                if (unique.Count > 0)
                {
                    value["unique"] = unique[i];
                }

                values.Add(value);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["values"] = values },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetProjectData;

        public override Guid ComponentGuid =>
            new Guid("dab27dc6-389f-440a-a3f4-14640ba50247");
    }
}
