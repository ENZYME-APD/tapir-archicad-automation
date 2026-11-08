using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    // ModifyCurtainWallFrames identifies a frame by frameId, not elementId, so
    // it does not fit ModifyElementsComponentBase.
    public class ModifyCurtainWallFramesComponent : ArchicadExecutorComponent
    {
        public override string CommandName => "ModifyCurtainWallFrames";

        public ModifyCurtainWallFramesComponent()
            : base(
                "ModifyCurtainWallFrames",
                "EXPERIMENTAL. Modify single Curtain Wall Frames by their GUID (the frames of " +
                "GetSubelementsOfHierarchicalElements): the frame contour (A1, A2, B1, B2), the frame class " +
                "and the frame type. Only the connected optional inputs are changed on the frames. The change " +
                "is written through the owner curtain wall and read back to check that Archicad applied it.",
                GroupNames.ElementModification)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "FrameGuids",
                "Identifiers of the Curtain Wall Frames to modify.");

            InNumbers(
                "A1",
                "Contour width inside (frameContour a1, as GetDetailsOfElements returns it). " +
                "Input only 1 to use the same value for all frames. Optional.");

            InNumbers(
                "A2",
                "Contour width outside (frameContour a2). Input only 1 to use the same value for all frames. Optional.");

            InNumbers(
                "B1",
                "Contour length inside (frameContour b1). Input only 1 to use the same value for all frames. Optional.");

            InNumbers(
                "B2",
                "Contour length outside (frameContour b2). Input only 1 to use the same value for all frames. Optional.");

            InIntegers(
                "ClassIndices",
                "The frame class (classID): 0 to 3 for the built-in classes, 4 and up for the frame classes " +
                "of the owner curtain wall. Input only 1 to use the same value for all frames. Optional.");

            InTexts(
                "FrameTypes",
                "The type of the frame: Invisible, GDL, InvisibleConnectedTurning or InvisibleConnectedStill. " +
                "Input only 1 to use the same value for all frames. Optional.");

            SetOptionality(new[] { 1, 2, 3, 4, 5, 6 });
        }

        protected override void AddOutputs()
        {
            OutErrorMessages(
                "Error message of each frame (empty when it was modified successfully).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<GH_ObjectWrapper> frameWrappers))
            {
                return;
            }

            var frameIds = ElementInputHelp.ElementGuids(
                this,
                frameWrappers,
                "FrameGuids");
            if (frameIds == null)
            {
                return;
            }

            var count = frameIds.Count;
            var a1 = da.OptionalList<double>(1);
            var a2 = da.OptionalList<double>(2);
            var b1 = da.OptionalList<double>(3);
            var b2 = da.OptionalList<double>(4);
            var classIndices = da.OptionalList<int>(5);
            var frameTypes = da.OptionalList<string>(6);

            if (!this.CheckSizes(
                    count,
                    "FrameGuids",
                    ("A1", a1.Count),
                    ("A2", a2.Count),
                    ("B1", b1.Count),
                    ("B2", b2.Count),
                    ("ClassIndices", classIndices.Count),
                    ("FrameTypes", frameTypes.Count)))
            {
                return;
            }

            var frames = new JArray();
            for (var i = 0; i < count; i++)
            {
                var frame = new JObject
                {
                    ["frameId"] = new JObject { ["guid"] = frameIds[i].Guid }
                };

                var frameContour = new JObject();
                if (a1.Count > 0)
                {
                    frameContour["a1"] = a1.At(i);
                }
                if (a2.Count > 0)
                {
                    frameContour["a2"] = a2.At(i);
                }
                if (b1.Count > 0)
                {
                    frameContour["b1"] = b1.At(i);
                }
                if (b2.Count > 0)
                {
                    frameContour["b2"] = b2.At(i);
                }
                if (frameContour.Count > 0)
                {
                    frame["frameContour"] = frameContour;
                }

                if (classIndices.Count > 0)
                {
                    frame["classIndex"] = classIndices.At(i);
                }
                if (frameTypes.Count > 0)
                {
                    frame["type"] = frameTypes.At(i);
                }

                frames.Add(frame);
            }

            SetCadValuesWithErrorMessages(
                CommandName,
                new JObject { ["framesWithDetails"] = frames },
                ToAddOn,
                da);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ModifyCurtainWallFrames;

        public override Guid ComponentGuid =>
            new Guid("8ff5a7e0-1dca-4423-ae87-5ef476528812");
    }
}
