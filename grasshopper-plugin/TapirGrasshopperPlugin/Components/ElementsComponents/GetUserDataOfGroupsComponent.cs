using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetUserDataOfGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetUserDataOfGroups";

        public GetUserDataOfGroupsComponent()
            : base(
                "GetUserDataOfGroups",
                "Get the user data an add-on attached to each given group: its version, platform sign, flags and " +
                "bytes (base64 encoded), and, when its version is 1 (the layout SetUserDataOfGroups writes), its " +
                "key-value pairs. Returns an error for groups that have no user data.",
                GroupNames.ElementGrouping)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "GroupGuids",
                "Identifiers of the groups.");
        }

        protected override void AddOutputs()
        {
            OutIntegers(
                "DataVersions",
                "Version of the user data of each group, as set by the add-on that attached it.");

            OutIntegers(
                "PlatformSigns",
                "Platform sign of the user data of each group, which tells the byte order it was written in.");

            OutIntegers(
                "Flags",
                "Flags of the user data of each group (APIUserDataFlag_FillWith and APIUserDataFlag_Pickup bits).");

            OutTexts(
                "Data",
                "Bytes of the user data of each group, base64 encoded (empty when it has no bytes).");

            OutTextTree(
                "Keys",
                "Keys of the key-value pairs of each group (one branch per group; empty unless the data version is 1).");

            OutTextTree(
                "Values",
                "Values of the key-value pairs of each group (one branch per group; empty unless the data version is 1).");

            OutErrorMessages(
                "Error message for each group (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> groupWrappers))
            {
                return;
            }

            var groups = new JArray();
            foreach (var wrapper in groupWrappers)
            {
                var id = GuidObject<GroupGuid>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    this.AddError("Invalid group identifier in the GroupGuids input.");
                    return;
                }
                groups.Add(
                    new JObject
                    {
                        ["groupId"] = new JObject { ["guid"] = id.Guid }
                    });
            }

            if (!TryGetCadResponse(
                    CommandName,
                    new JObject { ["groups"] = groups },
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            var dataVersions = new List<object>();
            var platformSigns = new List<object>();
            var flags = new List<object>();
            var data = new List<object>();
            var keys = new DataTree<object>();
            var values = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "userDataOfGroups");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                keys.EnsurePath(path);
                values.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    dataVersions.Add(null);
                    platformSigns.Add(null);
                    flags.Add(null);
                    data.Add(null);
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                var userData = item["userData"];
                dataVersions.Add(JsonOutputHelp.Scalar(userData, "dataVersion"));
                platformSigns.Add(JsonOutputHelp.Scalar(userData, "platformSign"));
                flags.Add(JsonOutputHelp.Scalar(userData, "flags"));
                data.Add(JsonOutputHelp.Scalar(userData, "data"));
                errors.Add("");

                if (userData?["pairs"] is JArray pairs)
                {
                    foreach (var pair in pairs)
                    {
                        keys.Add(JsonOutputHelp.Scalar(pair, "key"), path);
                        values.Add(JsonOutputHelp.Scalar(pair, "value"), path);
                    }
                }
            }

            da.SetDataList(0, dataVersions);
            da.SetDataList(1, platformSigns);
            da.SetDataList(2, flags);
            da.SetDataList(3, data);
            da.SetDataTree(4, keys);
            da.SetDataTree(5, values);
            da.SetDataList(6, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetUserDataOfGroups;

        public override Guid ComponentGuid =>
            new Guid("4e70e3dd-69a4-4695-b822-4eedf3bbe65b");
    }
}
