using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rhino.Geometry;
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Security.Cryptography.X509Certificates;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class Vector3D
    {
        [JsonProperty ("x")]
        public double X;

        [JsonProperty ("y")]
        public double Y;

        [JsonProperty ("z")]
        public double Z;
    }

    public class ElementWithMoveParameters
    {
        [JsonProperty ("elementId")]
        public ElementIdObj Element;

        [JsonProperty ("moveVector")]
        public Vector3D MoveVector;

        [JsonProperty ("copy")]
        public bool Copy;
    }

    public class MoveElementsParameters
    {
        [JsonProperty ("elementsWithMoveVectors")]
        public List<ElementWithMoveParameters> ElementsWithMoveParameters;
    }

    public class MoveElementsComponent : ArchicadExecutorComponent
    {
        public MoveElementsComponent ()
          : base (
                "Move Elements",
                "MoveElements",
                "Move elements",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementIds", "ElementIds", "Element ids to move.", GH_ParamAccess.list);
            pManager.AddVectorParameter ("Moving 3D Vectors", "MoveVectors", "The 3D vectors to move elements (input only 1 vector to move all elements with the same vector).", GH_ParamAccess.list);
            pManager.AddBooleanParameter ("Move copies", "MoveCopies", "Move copies of the elements.", GH_ParamAccess.item, @default: false);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj elements = ElementsObj.Create (DA, 0);
            if (elements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementIds failed to collect data.");
                return;
            }

            List<Vector3d> moveVectors = new List<Vector3d> ();
            if (!DA.GetDataList (1, moveVectors)) {
                return;
            }

            if (moveVectors.Count != 1 && moveVectors.Count != elements.Elements.Count) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "The size of the input MoveVectors must be 1 or equal to the size of the input ElementIds.");
                return;
            }

            bool copy = false;
            if (!DA.GetData (2, ref copy)) {
                return;
            }

            MoveElementsParameters moveElementsParameters = new MoveElementsParameters () {
                ElementsWithMoveParameters = new List<ElementWithMoveParameters> ()
            };
            for (int i = 0; i < elements.Elements.Count; i++) {
                ElementIdItemObj element = elements.Elements[i];
                Vector3d moveVector = moveVectors[moveVectors.Count == 1 ? 0 : i];
                moveElementsParameters.ElementsWithMoveParameters.Add (new ElementWithMoveParameters () {
                    Element = element.ElementId,
                    MoveVector = new Vector3D () {
                        X = moveVector.X,
                        Y = moveVector.Y,
                        Z = moveVector.Z
                    },
                    Copy = copy
                });
            }
            JObject moveElementsParametersObj = JObject.FromObject (moveElementsParameters);
            CommandResponse response = SendArchicadAddOnCommand ("TapirCommand", "MoveElements", moveElementsParametersObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.MoveElements;

        public override Guid ComponentGuid => new Guid ("e37db5dc-9ac6-42e6-836c-3c2b04b15d95");
    }
}
