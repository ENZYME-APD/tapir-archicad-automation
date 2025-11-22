using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
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
                "Name",
                "Name of the layer.");

            InBooleans(
                "IsHidden",
                "Visibility of the layer.");

            InBooleans(
                "IsLocked",
                "Lock states of the layer.");

            InBooleans(
                "IsWireframe",
                "Wireframe flag of the layer.");

            InIntegers(
                "IntersectionGroup",
                "Intersection group of the layer.");
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

            var names = new List<string>();
            if (!da.GetDataList(
                    1,
                    names))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input AttributeNames failed to collect data.");
                return;
            }

            if (attributes.Attributes.Count != names.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The number of AttributeGuids and Names must be the same.");
                return;
            }

            var isHiddenLayers = new List<bool>();
            if (!da.GetDataList(
                    2,
                    isHiddenLayers))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IsHidden failed to collect data.");
                return;
            }

            var isLockedLayers = new List<bool>();
            if (!da.GetDataList(
                    3,
                    isLockedLayers))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IsLocked failed to collect data.");
                return;
            }

            var isWireframeLayers = new List<bool>();
            if (!da.GetDataList(
                    4,
                    isWireframeLayers))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IsWireframe failed to collect data.");
                return;
            }

            var intersectionGroupsOfLayers = new List<int>();
            if (!da.GetDataList(
                    5,
                    intersectionGroupsOfLayers))
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input IntersectionGroup failed to collect data.");
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
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The number of items in the input lists must match the number of input layers.");
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
                    new LayerDataObj()
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

            SetArchiCadValues(
                CommandName,
                layerDataArray);
        }

        public override Guid ComponentGuid =>
            new Guid("7e336988-3756-42e1-bc27-8d4e83e21ae5");
    }
}