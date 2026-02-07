using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using Rhino.Geometry;
using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class HighlightElementsComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "HighlightElements";

        public HighlightElementsComponent()
            : base(
                "HighlightElements",
                "Highlight Elements.",
                GroupNames.Elements)
        {
            Params.ParameterSourcesChanged += OnParameterSourcesChanged;
        }

        public override void DocumentContextChanged(
            GH_Document document,
            GH_DocumentContext context)
        {
            base.DocumentContextChanged(
                document,
                context);
            if (context == GH_DocumentContext.Close ||
                context == GH_DocumentContext.Unloaded)
            {
                ClearHighlight();
            }
        }

        public override void RemovedFromDocument(
            GH_Document document)
        {
            base.RemovedFromDocument(document);
            ClearHighlight();
        }

        public override void AddedToDocument(
            GH_Document document)
        {
            var counter = 0;
            var doc = OnPingDocument();
            if (doc != null)
            {
                foreach (var obj in doc.Objects)
                    if (obj.ComponentGuid == ComponentGuid)
                    {
                        counter++;
                        if (counter > 1)
                        {
                            break;
                        }
                    }
            }

            if (counter > 1)
            {
                doc.RemoveObject(
                    this,
                    true);
                MessageBox.Show(
                    "Only one instance can be created.",
                    "Highlight Elements");
            }
            else
            {
                base.AddedToDocument(document);
            }
        }

        protected override void AddInputs()
        {
            InBoolean(
                "Enable",
                "Enable highlight.",
                true);

            InGenericTree(
                "ElementGuids",
                "Elements to highlight.");

            InColorTree(
                "HighligtedColors",
                "Colors for the elements.",
                Color.Blue);

            InColor(
                "NonHighligtedColor",
                "Color for the non-highlighted elements.",
                Color.White);

            InBoolean(
                "NonHighligtedWireframe",
                "Switch non-highlighted elements in the 3D window to wireframe",
                true);
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            var enabled = true;
            if (da.GetData(
                    0,
                    ref enabled) && !enabled)
            {
                ClearHighlight();
                return;
            }

            if (!da.TryGetTree(
                    1,
                    out GH_Structure<IGH_Goo> elementTree) || !elementTree
                    .FlattenData()
                    .Select(x => x.ToWrapper())
                    .ToList()
                    .TryBuildObject(out ElementsObject input))
            {
                return;
            }

            if (!da.TryGetTree(
                    2,
                    out GH_Structure<GH_Colour> colorTree))
            {
                return;
            }

            var colorTreeContainsOnlyOneColor = colorTree.Branches.Count == 1 && colorTree.Branches[0].Count == 1;
            if (!colorTreeContainsOnlyOneColor && !colorTree.EqualsTo(elementTree))
            {
                this.AddError("Unequal tree structures!");
                return;
            }

            if (!da.TryGet(
                    3,
                    out GH_Colour nonHighlightedColor))
            {
                return;
            }

            if (!da.TryGet(
                    4,
                    out bool wireframe3D))
            {
                return;
            }

            var highlightedColors = colorTreeContainsOnlyOneColor
                ? Enumerable.Repeat(
                    colorTree.Branches[0][0],
                    input.Elements.Count)
                : colorTree.FlattenData();

            var highlightElements = new HighlightElementsObj
            {
                Elements = input.Elements,
                HighlightedColors = Helps.Convert.ToRgb(
                    highlightedColors),
                NonHighlightedColor = Helps.Convert.ToRgb(
                    nonHighlightedColor),
                Wireframe3D = wireframe3D
            };

            SetCadValues(
                CommandName,
                highlightElements,
                ToAddOn);
        }

        private void ClearHighlight()
        {
            SetCadValues(
                CommandName,
                new HighlightElementsObj(),
                ToAddOn);
        }

        public void OnParameterSourcesChanged(
            object source,
            EventArgs e)
        {
            foreach (var param in Params.Input)
            {
                if (!param.Optional && param.SourceCount == 0)
                {
                    ClearHighlight();
                    break;
                }
            }
        }

        protected override Bitmap Icon => Properties.Resources.HighlightElems;

        public override Guid ComponentGuid =>
            new Guid("9ba098dd-63c5-4126-b4cc-3caa56082c8f");
    }
}