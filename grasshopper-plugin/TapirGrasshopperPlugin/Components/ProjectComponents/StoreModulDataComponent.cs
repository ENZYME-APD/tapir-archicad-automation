using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    public class StoreModulDataComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "StoreModulData";

        public StoreModulDataComponent()
            : base(
                "StoreModulData",
                "Creates or overwrites module data: text (for example JSON) saved in the project under a name, " +
                "with an optional data version. Prefix the names with the name of your tool (for example " +
                "'my-tool/settings') so scripts do not overwrite each other's data. Storing module data is not " +
                "an undoable step.",
                GroupNames.Project)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Names of the module data; the empty name is the default section.");

            InTexts(
                "Contents",
                "Content of each module data, for example a JSON text (input only 1 to use the same content for all).");

            InIntegers(
                "DataVersions",
                "Version number stored with each module data, for example to tell the formats of your content " +
                "apart (input only 1 to use the same version for all). 1 when empty.");

            SetOptionality(2);
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each module data (empty when it was stored).");
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

            da.TryGetList(
                2,
                out List<int> dataVersions);
            if (!this.CheckSizes(
                    names.Count,
                    "Names",
                    ("Contents", contents.Count),
                    ("DataVersions", dataVersions.Count)))
            {
                return;
            }
            if (contents.Count == 0)
            {
                this.AddError("The input Contents must contain at least one item.");
                return;
            }

            var modulData = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject
                {
                    ["name"] = names[i],
                    ["content"] = contents.At(i)
                };

                if (dataVersions.Count > 0)
                {
                    item["dataVersion"] = dataVersions.At(i);
                }

                modulData.Add(item);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["modulData"] = modulData },
                ToAddOn,
                da);
        }

        // Shares the icon of SetProjectData, the other text data saved in the project.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetProjectData;

        public override Guid ComponentGuid =>
            new Guid("9998da95-5a6a-44ea-a70f-7519c4d9fd2f");
    }
}
