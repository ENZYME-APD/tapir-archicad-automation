using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class CreateLayersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLayers";

        public CreateLayersComponent()
            : base(
                "CreateLayers",
                "Create or overwrite Layer attributes based on the given parameters.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InTexts(
                "Names",
                "Name of each layer to create. If OverwriteExisting is true, " +
                "the existing layer with the same name will be overwritten.");

            InBooleans(
                "IsHidden",
                "Visibility state of each layer (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IsLocked",
                "Lock state of each layer (input only 1 to use the same value for all). Optional.");

            InBooleans(
                "IsWireframe",
                "Wireframe flag of each layer (input only 1 to use the same value for all). Optional.");

            InIntegers(
                "IntersectionGroups",
                "Intersection group of each layer (input only 1 to use the same value for all). Optional.");

            InGenerics(
                "AttributeGuids",
                "Identifier of the existing layer to overwrite for each item. " +
                "Optional; ignored when OverwriteExisting is false.");

            InBoolean(
                "OverwriteExisting",
                "Overwrite the existing layer with the same name or identifier.",
                false);

            SetOptionality(new[] { 1, 2, 3, 4, 5 });
        }

        protected override void AddOutputs()
        {
            AttributeCreateResult.AddOutputs(this);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<string> names))
            {
                return;
            }

            da.TryGetList(
                1,
                out List<bool> isHiddenLayers);

            da.TryGetList(
                2,
                out List<bool> isLockedLayers);

            da.TryGetList(
                3,
                out List<bool> isWireframeLayers);

            da.TryGetList(
                4,
                out List<int> intersectionGroupsOfLayers);

            foreach (var pair in new (string Name, int Count)[]
                     {
                         ("IsHidden", isHiddenLayers.Count),
                         ("IsLocked", isLockedLayers.Count),
                         ("IsWireframe", isWireframeLayers.Count),
                         ("IntersectionGroups", intersectionGroupsOfLayers.Count)
                     })
            {
                if (pair.Count > 1 &&
                    pair.Count != names.Count)
                {
                    this.AddError(
                        $"The size of the input {pair.Name} must be 0, 1 or equal to the size of the input Names.");
                    return;
                }
            }

            da.TryGetList(
                5,
                out List<GH_ObjectWrapper> guidWrappers);

            var attributeGuids = new List<AttributeGuidObject>();
            if (guidWrappers.Count > 0)
            {
                if (!guidWrappers.TryBuildObject(
                        out AttributesObject attributes))
                {
                    this.AddError(
                        "Invalid attribute identifier in the AttributeGuids input.");
                    return;
                }

                if (attributes.Attributes.Count != names.Count)
                {
                    this.AddError(
                        "The size of the input AttributeGuids must be 0 or equal to the size of the input Names.");
                    return;
                }

                foreach (var wrapper in attributes.Attributes)
                {
                    attributeGuids.Add(wrapper.AttributeId);
                }
            }

            da.TryGet(
                6,
                out bool overwriteExisting);

            var items = new JArray();
            for (var i = 0; i < names.Count; i++)
            {
                var item = new JObject { ["name"] = names[i] };

                if (attributeGuids.Count > 0)
                {
                    item["attributeId"] = new JObject
                    {
                        ["guid"] = attributeGuids[i].Guid
                    };
                }

                if (isHiddenLayers.Count > 0)
                {
                    item["isHidden"] =
                        isHiddenLayers[isHiddenLayers.Count == 1 ? 0 : i];
                }

                if (isLockedLayers.Count > 0)
                {
                    item["isLocked"] =
                        isLockedLayers[isLockedLayers.Count == 1 ? 0 : i];
                }

                if (isWireframeLayers.Count > 0)
                {
                    item["isWireframe"] =
                        isWireframeLayers[isWireframeLayers.Count == 1 ? 0 : i];
                }

                if (intersectionGroupsOfLayers.Count > 0)
                {
                    item["intersectionGroupNr"] =
                        intersectionGroupsOfLayers[
                            intersectionGroupsOfLayers.Count == 1 ? 0 : i];
                }

                items.Add(item);
            }

            var parameters = new JObject
            {
                ["layerDataArray"] = items,
                ["overwriteExisting"] = overwriteExisting
            };

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out var response))
            {
                return;
            }

            AttributeCreateResult.SetOutputs(
                da,
                response);
        }

        // TODO: add a dedicated CreateLayers.png resource; the binary asset
        // could not be added in this automated change.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.SetLayers;

        public override Guid ComponentGuid =>
            new Guid("b3a1c6f2-58d9-4e07-9c44-2f6a71e0d5b8");
    }
}
