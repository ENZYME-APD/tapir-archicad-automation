using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.NavigatorComponents
{
    public class GetCamerasComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetCameras";

        public GetCamerasComponent()
            : base(
                "GetCameras",
                "Get the Camera Sets (camera paths) of the project with their name, whether the set is the " +
                "active one, its current camera, and its cameras in path order. Each camera has its element id, " +
                "whether it is active in the path, and its perspective in the inputs Set3DProjectionSettings " +
                "takes, so a camera's values passed there show its view in the 3D window. Archicad cameras are " +
                "always perspective; they have no name of their own.",
                GroupNames.Navigator)
        {
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "CameraSetGuids",
                "Element identifier of each Camera Set.");

            OutTexts(
                "CameraSetNames",
                "The name of each Camera Set, as the navigator shows it.");

            OutBooleans(
                "IsActiveCameraSet",
                "True for the active Camera Set.");

            OutGenerics(
                "ActiveCameraGuids",
                "The current camera of each Camera Set (empty when the set has none).");

            OutGenericTree(
                "CameraGuids",
                "Element identifiers of the cameras of each set, in the order of the path (one branch per set).");

            OutBooleanTree(
                "IsActiveInPath",
                "True when the camera is active in the path of its Camera Set (one branch per set).");

            OutPointTree(
                "CameraPositions",
                "Position of each camera (one branch per set).");

            OutPointTree(
                "TargetPositions",
                "Position of the target point each camera looks at (one branch per set).");

            OutNumbers(
                "Distances",
                "Distance between each camera and its target (one branch per set).");

            OutNumbers(
                "Azimuths",
                "Azimuth angle of each camera (one branch per set).");

            OutNumbers(
                "ViewCones",
                "View cone angle of each camera (one branch per set).");

            OutNumbers(
                "RollAngles",
                "Roll angle of each camera (one branch per set).");

            OutBooleanTree(
                "IsTwoPointPerspective",
                "True for a two-point perspective (vertical edges stay vertical), for each camera (one branch per set).");

            OutTextTree(
                "Sun_JSON",
                "The sun position of each camera as a JSON object, in the shape the Sun_JSON input of " +
                "Set3DProjectionSettings takes (one branch per set).");
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

            var setGuids = new List<object>();
            var setNames = new List<string>();
            var setIsActive = new List<bool>();
            var activeCameraGuids = new List<object>();
            var cameraGuids = new DataTree<object>();
            var isActiveInPath = new DataTree<object>();
            var cameraPositions = new DataTree<object>();
            var targetPositions = new DataTree<object>();
            var distances = new DataTree<object>();
            var azimuths = new DataTree<object>();
            var viewCones = new DataTree<object>();
            var rollAngles = new DataTree<object>();
            var isTwoPointPerspective = new DataTree<object>();
            var suns = new DataTree<object>();

            var cameraSets = JsonOutputHelp.Items(
                response,
                "cameraSets");
            for (var i = 0; i < cameraSets.Count; i++)
            {
                var cameraSet = cameraSets[i];
                setGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(cameraSet, "cameraSetElementId"));
                setNames.Add(cameraSet["name"]?.ToString());
                setIsActive.Add(cameraSet.Value<bool?>("isActive") ?? false);
                activeCameraGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(cameraSet, "activeCameraElementId"));

                var path = new GH_Path(i);
                foreach (var tree in new[]
                         {
                             cameraGuids, isActiveInPath, cameraPositions, targetPositions, distances,
                             azimuths, viewCones, rollAngles, isTwoPointPerspective, suns
                         })
                {
                    tree.EnsurePath(path);
                }

                if (!(cameraSet["cameras"] is JArray cameras))
                {
                    continue;
                }

                foreach (var camera in cameras)
                {
                    var perspective = camera["perspective"];
                    cameraGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(camera, "cameraElementId"), path);
                    isActiveInPath.Add(JsonOutputHelp.Scalar(camera, "isActiveInPath"), path);
                    cameraPositions.Add(JsonOutputHelp.Point(perspective?["cameraPosition"]), path);
                    targetPositions.Add(JsonOutputHelp.Point(perspective?["targetPosition"]), path);
                    distances.Add(JsonOutputHelp.Scalar(perspective, "distance"), path);
                    azimuths.Add(JsonOutputHelp.Scalar(perspective, "azimuth"), path);
                    viewCones.Add(JsonOutputHelp.Scalar(perspective, "viewCone"), path);
                    rollAngles.Add(JsonOutputHelp.Scalar(perspective, "rollAngle"), path);
                    isTwoPointPerspective.Add(JsonOutputHelp.Scalar(perspective, "isTwoPointPersp"), path);
                    suns.Add(perspective?["sun"]?.ToString(Formatting.None), path);
                }
            }

            da.SetDataList(0, setGuids);
            da.SetDataList(1, setNames);
            da.SetDataList(2, setIsActive);
            da.SetDataList(3, activeCameraGuids);
            da.SetDataTree(4, cameraGuids);
            da.SetDataTree(5, isActiveInPath);
            da.SetDataTree(6, cameraPositions);
            da.SetDataTree(7, targetPositions);
            da.SetDataTree(8, distances);
            da.SetDataTree(9, azimuths);
            da.SetDataTree(10, viewCones);
            da.SetDataTree(11, rollAngles);
            da.SetDataTree(12, isTwoPointPerspective);
            da.SetDataTree(13, suns);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetCameras;

        public override Guid ComponentGuid =>
            new Guid("fa70538a-f9a8-4b7b-8e42-dc8e527780e4");
    }
}
