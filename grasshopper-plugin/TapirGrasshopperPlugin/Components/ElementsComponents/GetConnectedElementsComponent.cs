using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetConnectedElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetConnectedElements";

        public GetConnectedElementsComponent()
            : base(
                "ConnectedElements",
                "Gets the connected elements of the given elements.",
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

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ConnectedElementGuids",
                "Connected Elements with the given type.");
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
            if (!ElementsObj.TryCreate(
                    da,
                    0,
                    out ElementsObj inputElements))
            {
                return;
            }

            if (!da.TryGetItem(
                    1,
                    out string eType))
            {
                return;
            }

            var parameters = new GetConnectedElementsParameters()
            {
                Elements = inputElements.Elements,
                ConnectedElementType = eType
            };

            if (!TryGetConvertedResponse(
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