using Grasshopper.Kernel;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Vector3D
    {
        [JsonProperty("x")]
        public double X;

        [JsonProperty("y")]
        public double Y;

        [JsonProperty("z")]
        public double Z;
    }

    public class ElementWithMoveParameters
    {
        [JsonProperty("elementId")]
        public ElementIdObj Element;

        [JsonProperty("moveVector")]
        public Vector3D MoveVector;

        [JsonProperty("copy")]
        public bool Copy;
    }

    public class MoveElementsParameters
    {
        [JsonProperty("elementsWithMoveVectors")]
        public List<ElementWithMoveParameters> ElementsWithMoveParameters;
    }

    public class MoveElementsComponent : ArchicadExecutorComponent
    {
        public static string Doc => "Move elements";
        public override string CommandName => "MoveElements";

        public MoveElementsComponent()
            : base(
                "Move Elements",
                "MoveElements",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void RegisterInputParams(
            GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ElementGuids",
                "ElementGuids",
                "Elements ids to move.",
                GH_ParamAccess.list);
            pManager.AddVectorParameter(
                "Moving 3D Vectors",
                "MoveVectors",
                "The 3D vectors to move elements (input only 1 vector to move all elements with the same vector).",
                GH_ParamAccess.list);
            pManager.AddBooleanParameter(
                "Move copies",
                "MoveCopies",
                "Move copies of the elements.",
                GH_ParamAccess.item,
                @default: false);
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var elements = ElementsObj.Create(
                da,
                0);
            if (elements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var moveVectors = new List<Vector3d>();
            if (!da.GetDataList(
                    1,
                    moveVectors))
            {
                return;
            }

            if (moveVectors.Count != 1 &&
                moveVectors.Count != elements.Elements.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The size of the input MoveVectors must be 1 or equal to the size of the input ElementGuids.");
                return;
            }

            var copy = false;
            if (!da.GetData(
                    2,
                    ref copy))
            {
                return;
            }

            var moveElementsParameters = new MoveElementsParameters()
            {
                ElementsWithMoveParameters =
                    new List<ElementWithMoveParameters>()
            };
            for (var i = 0; i < elements.Elements.Count; i++)
            {
                var element = elements.Elements[i];
                var moveVector = moveVectors[moveVectors.Count == 1 ? 0 : i];
                moveElementsParameters.ElementsWithMoveParameters.Add(
                    new ElementWithMoveParameters()
                    {
                        Element = element.ElementId,
                        MoveVector = new Vector3D()
                        {
                            X = moveVector.X,
                            Y = moveVector.Y,
                            Z = moveVector.Z
                        },
                        Copy = copy
                    });
            }

            GetResponse(
                CommandName,
                moveElementsParameters);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.MoveElements;

        public override Guid ComponentGuid =>
            new Guid("e37db5dc-9ac6-42e6-836c-3c2b04b15d95");
    }
}