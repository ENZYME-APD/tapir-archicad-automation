using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;
using TapirGrasshopperPlugin.Types.MEP;

namespace TapirGrasshopperPlugin.Components.MEPComponents
{
    public class GetMEPSystemGroupsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetMEPSystemGroups";

        public GetMEPSystemGroupsComponent()
            : base(
                "GetMEPSystemGroups",
                "Get the system groups of the MEP physical systems of the project: the name of each group and, " +
                "per group, its physical systems with their name, root element and MEP system attribute. " +
                "Tree outputs have one branch per system group. " +
                "Available from Archicad 29.",
                GroupNames.MEP)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "SystemGroupGuids",
                "Identifier of each system group.");

            OutTexts(
                "SystemGroupNames",
                "Name of each system group.");

            OutGenericTree(
                "PhysicalSystemGuids",
                "Identifiers of the physical systems of each system group (one branch per group).");

            OutTextTree(
                "PhysicalSystemNames",
                "Name of each physical system (one branch per group).");

            OutGenericTree(
                "RootElementGuids",
                "Identifier of the MEP element each physical system starts from (null when it has none; one branch per group).");

            OutGenericTree(
                "MEPSystemGuids",
                "Identifier of the MEP system attribute used as the category of each physical system " +
                "(null when it has none; one branch per group).");

            OutTextTree(
                "PhysicalSystemErrorMessages",
                "Error message of each physical system (empty when it was read successfully; one branch per group).");

            OutErrorMessages(
                "Error message of each system group (empty when it was read successfully).");
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

            var groupGuids = new List<object>();
            var groupNames = new List<object>();
            var systemGuids = new DataTree<object>();
            var systemNames = new DataTree<object>();
            var rootElementGuids = new DataTree<object>();
            var mepSystemGuids = new DataTree<object>();
            var systemErrors = new DataTree<object>();
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(response, "systemGroups");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                systemGuids.EnsurePath(path);
                systemNames.EnsurePath(path);
                rootElementGuids.EnsurePath(path);
                mepSystemGuids.EnsurePath(path);
                systemErrors.EnsurePath(path);

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    groupGuids.Add(null);
                    groupNames.Add(null);
                    continue;
                }

                errors.Add("");
                groupGuids.Add(SystemGroupIdOf(item["systemGroupId"]));
                groupNames.Add(JsonOutputHelp.Scalar(item, "name"));

                if (!(item["physicalSystems"] is JArray physicalSystems))
                {
                    continue;
                }

                foreach (var system in physicalSystems)
                {
                    // A physical system the add-on could not read is an error
                    // item in the array; its place is kept with nulls, so the
                    // branches stay aligned with each other.
                    if (JsonOutputHelp.IsError(system))
                    {
                        systemErrors.Add(JsonOutputHelp.ErrorMessage(system), path);
                        systemGuids.Add(null, path);
                        systemNames.Add(null, path);
                        rootElementGuids.Add(null, path);
                        mepSystemGuids.Add(null, path);
                        continue;
                    }

                    systemErrors.Add("", path);
                    systemGuids.Add(PhysicalSystemIdOf(system["physicalSystemId"]), path);
                    systemNames.Add(JsonOutputHelp.Scalar(system, "name"), path);
                    rootElementGuids.Add(ElectricalComponentHelp.ElementIdOf(system["rootElementId"]), path);
                    mepSystemGuids.Add(AttributeIdOf(system["mepSystemId"]), path);
                }
            }

            da.SetDataList(0, groupGuids);
            da.SetDataList(1, groupNames);
            da.SetDataTree(2, systemGuids);
            da.SetDataTree(3, systemNames);
            da.SetDataTree(4, rootElementGuids);
            da.SetDataTree(5, mepSystemGuids);
            da.SetDataTree(6, systemErrors);
            da.SetDataList(7, errors);
        }

        private static MEPSystemGroupGuidWrapper SystemGroupIdOf(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }
            return new MEPSystemGroupGuidWrapper
            {
                SystemGroupId = new MEPSystemGroupGuid { Guid = guid }
            };
        }

        private static MEPPhysicalSystemGuidWrapper PhysicalSystemIdOf(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }
            return new MEPPhysicalSystemGuidWrapper
            {
                PhysicalSystemId = new MEPPhysicalSystemGuid { Guid = guid }
            };
        }

        private static AttributeGuidWrapper AttributeIdOf(
            JToken idObject)
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            if (guid == null)
            {
                return null;
            }
            return new AttributeGuidWrapper
            {
                AttributeId = new AttributeGuidObject { Guid = guid }
            };
        }

        // Shares the icon of GetMEPDistributionSystems until it gets its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetMEPDistributionSystems;

        public override Guid ComponentGuid =>
            new Guid("5600da8e-a01c-4993-a349-558b8b0f6b63");
    }
}
