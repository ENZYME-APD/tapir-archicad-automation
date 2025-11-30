using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Attributes;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class SetLayersComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "CreateLayers";

        public SetLayersComponent()
            : base(
                "SetLayers",
                "Set the details of layers.",
                GroupNames.Attributes)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "AttributeGuids",
                "List of layer attribute Guids.");

            InTexts(
                "Names",
                "Name of the layers.");

            InBooleans(
                "IsHidden",
                "Visibility of the layers.");

            InBooleans(
                "IsLocked",
                "Lock states of the layers.");

            InBooleans(
                "IsWireframe",
                "Wireframe flag of the layers.");

            InIntegers(
                "IntersectionGroups",
                "Intersection group of the layers.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!AttributesObj.TryCreate(
                    da,
                    0,
                    out AttributesObj attributes))
            {
                return;
            }

            if (!da.TryGetItems(
                    1,
                    out List<string> names))
            {
                return;
            }

            if (attributes.Attributes.Count != names.Count)
            {
                this.AddError("Attribute to Name count mismatch!");
                return;
            }

            if (!da.TryGetItems(
                    2,
                    out List<bool> isHiddenLayers))
            {
                return;
            }


            if (!da.TryGetItems(
                    3,
                    out List<bool> isLockedLayers))
            {
                return;
            }

            if (!da.TryGetItems(
                    4,
                    out List<bool> isWireframeLayers))
            {
                return;
            }


            if (!da.TryGetItems(
                    5,
                    out List<int> intersectionGroupsOfLayers))
            {
                return;
            }

            if ((names.Count != isHiddenLayers.Count &&
                 isHiddenLayers.Count != 1) ||
                (names.Count != isLockedLayers.Count &&
                 isLockedLayers.Count != 1) ||
                (names.Count != isWireframeLayers.Count &&
                 isWireframeLayers.Count != 1) ||
                (names.Count != intersectionGroupsOfLayers.Count &&
                 intersectionGroupsOfLayers.Count != 1))
            {
                this.AddError("Tree branch count inequality!");
                return;
            }

            var layerDataArray = new LayerDataArrayObj()
            {
                LayerDataArray = new List<LayerDataObj>(),
                OverwriteExisting = true
            };

            var index = 0;

            foreach (var attributeId in attributes.Attributes)
            {
                layerDataArray.LayerDataArray.Add(
                    new LayerDataObj
                    {
                        AttributeId = attributeId.AttributeId,
                        Name = names[index],
                        IsHidden =
                            isHiddenLayers.Count == 1
                                ? isHiddenLayers[0]
                                : isHiddenLayers[index],
                        IsLocked =
                            isLockedLayers.Count == 1
                                ? isLockedLayers[0]
                                : isLockedLayers[index],
                        IsWireframe =
                            isWireframeLayers.Count == 1
                                ? isWireframeLayers[0]
                                : isWireframeLayers[index],
                        IntersectionGroupNr =
                            intersectionGroupsOfLayers.Count == 1
                                ? intersectionGroupsOfLayers[0]
                                : intersectionGroupsOfLayers[index]
                    });
                index++;
            }

            SetValues(
                CommandName,
                layerDataArray,
                SendToAddOn);
        }

        public override Guid ComponentGuid =>
            new Guid("7e336988-3756-42e1-bc27-8d4e83e21ae5");
    }
}