using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetConnectedElementsParameters
    {
        [JsonProperty("elements")]
        public List<ElementIdItemObj> Elements;

        [JsonProperty("connectedElementType")]
        public string ConnectedElementType;
    }

    public class ConnectedElementsObj
    {
        [JsonProperty("connectedElements")]
        public List<ElementsObj> ConnectedElements;
    }

    public class GetConnectedElementsComponent : ArchicadAccessorComponent
    {
        public static string Doc =>
            "Gets the connected elements of the given elements.";

        public override string CommandName => "GetConnectedElements";

        public GetConnectedElementsComponent()
            : base(
                "Connected Elements",
                "ConnectedElems",
                Doc,
                GroupNames.Elements)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Elements ids of hierarchical elements to get subelements of.");

            InText(
                "ConnectedElemType",
                "Type of connected elements.");
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pManager)
        {
            pManager.AddGenericParameter(
                "ConnectedElementGuids",
                "ConnectedElementGuids",
                "Connected Elements with the given type.",
                GH_ParamAccess.tree);
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            new ElementTypeValueList(ElementTypeValueListType.SubElementsOnly)
                .AddAsSource(
                    this,
                    1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var inputElements = ElementsObj.Create(
                da,
                0);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var elemType = "";
            if (!da.GetData(
                    1,
                    ref elemType))
            {
                return;
            }

            var parameters = new GetConnectedElementsParameters()
            {
                Elements = inputElements.Elements,
                ConnectedElementType = elemType
            };

            if (!GetConvertedResponse(
                    CommandName,
                    parameters,
                    out ConnectedElementsObj connectedElementsObj))
            {
                return;
            }

            var connectedElementsTree = new DataTree<ElementIdItemObj>();

            for (var i = 0;
                 i < connectedElementsObj.ConnectedElements.Count;
                 i++)
            {
                var connectedElements =
                    connectedElementsObj.ConnectedElements[i];
                connectedElementsTree.AddRange(
                    connectedElements.Elements,
                    new GH_Path(i));
            }

            da.SetDataTree(
                0,
                connectedElementsTree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConnectedElements;

        public override Guid ComponentGuid =>
            new Guid("f857d496-6c7f-4b15-928a-d900a85dea32");
    }
}