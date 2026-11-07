using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using System;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetConnectedElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetConnectedElements";

        public GetConnectedElementsComponent()
            : base(
                "ConnectedElements",
                "Gets the connected elements of the given elements.",
                GroupNames.ElementDetails)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the owner elements, for example the Walls to get the openings of.");

            InText(
                "ConnectedElementType",
                "Type of the connected elements: Window or Door for the openings of a Wall, " +
                "Label for the labels of an element, or a subelement type of a Curtain Wall, Stair or Railing.");
        }

        protected override void AddOutputs()
        {
            OutGenericTree(
                "ConnectedElementGuids",
                "Connected elements of the given type, one branch per owner element.");
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            base.AddedToDocument(document);

            // Window and Door are the most common connected types (the openings
            // of a Wall), so the list offers the main element types too and
            // starts on Window.
            new ElementTypeValueList(
                    ElementTypeValueListType.AllElements,
                    MainElementType.Window.ToString())
                .AddAsSource(
                    this,
                    1);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject input))
            {
                return;
            }

            if (!da.TryGet(
                    1,
                    out string elementType))
            {
                return;
            }

            if (!TryGetConvertedCadValues(
                    CommandName,
                    new GetConnectedElementsParameters
                    {
                        Elements = input.Elements,
                        ConnectedElementType = elementType
                    },
                    ToAddOn,
                    JHelp.Deserialize<ConnectedElementsObject>,
                    out ConnectedElementsObject response))
            {
                return;
            }

            var tree = new DataTree<ElementGuidWrapper>();

            for (var i = 0; i < response.ConnectedElements.Count; i++)
            {
                tree.AddRange(
                    response.ConnectedElements[i].Elements,
                    new GH_Path(i));
            }

            da.SetDataTree(
                0,
                tree);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.ConnectedElements;

        public override Guid ComponentGuid =>
            new Guid("f857d496-6c7f-4b15-928a-d900a85dea32");
    }
}