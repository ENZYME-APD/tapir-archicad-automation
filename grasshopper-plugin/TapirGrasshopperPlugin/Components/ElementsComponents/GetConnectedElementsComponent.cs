using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using TapirGrasshopperPlugin.Utilities;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetConnectedElementsParameters
    {
        [JsonProperty ("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty ("connectedElementType")]
        public string ConnectedElementType;
    }

    public class ConnectedElementsObj
    {
        [JsonProperty ("connectedElements")]
        public List<ElementsObj> ConnectedElements;
    }

    public class GetConnectedElementsComponent : ArchicadAccessorComponent
    {
        public GetConnectedElementsComponent ()
          : base (
                "Connected Elements",
                "ConnectedElems",
                "Gets the connected elements of the given elements.",
                "Elements"
            )
        {
        }

        protected override void RegisterInputParams (GH_InputParamManager pManager)
        {
            pManager.AddGenericParameter ("ElementGuids", "ElementGuids", "Element ids of hierarchical elements to get subelements of.", GH_ParamAccess.list);
            pManager.AddTextParameter ("ConnectedElemType", "ConnectedElemType", "Type of connected elements.", GH_ParamAccess.item);
        }

        protected override void RegisterOutputParams (GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter ("ConnectedElementGuids", "ConnectedElementGuids", "Connected Elements with the given type.", GH_ParamAccess.tree);
        }

        public override void AddedToDocument (GH_Document document)
        {
            base.AddedToDocument (document);

            new ElementTypeValueList (ElementTypeValueListType.SubElementsOnly).AddAsSource (this, 1);
        }

        protected override void Solve (IGH_DataAccess DA)
        {
            ElementsObj inputElements = ElementsObj.Create (DA, 0);
            if (inputElements == null) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, "Input ElementGuids failed to collect data.");
                return;
            }

            string elemType = "";
            if (!DA.GetData (1, ref elemType)) {
                return;
            }

            GetConnectedElementsParameters parameters = new GetConnectedElementsParameters () {
                Elements = inputElements.Elements,
                ConnectedElementType = elemType
            };
            JObject parametersObj = JObject.FromObject (parameters);
            CommandResponse response = SendArchicadAddOnCommand ("GetConnectedElements", parametersObj);
            if (!response.Succeeded) {
                AddRuntimeMessage (GH_RuntimeMessageLevel.Error, response.GetErrorMessage ());
                return;
            }

            ConnectedElementsObj connectedElementsObj = response.Result.ToObject<ConnectedElementsObj> ();
            DataTree<ElementIdItemObj> connectedElementsTree = new DataTree<ElementIdItemObj> ();
            for (int i = 0; i < connectedElementsObj.ConnectedElements.Count; i++) {
                ElementsObj connectedElements = connectedElementsObj.ConnectedElements[i];
                connectedElementsTree.AddRange (connectedElements.Elements, new GH_Path (i));
            }
            DA.SetDataTree (0, connectedElementsTree);
        }

        protected override System.Drawing.Bitmap Icon => TapirGrasshopperPlugin.Properties.Resources.ConnectedElements;

        public override Guid ComponentGuid => new Guid ("f857d496-6c7f-4b15-928a-d900a85dea32");
    }
}