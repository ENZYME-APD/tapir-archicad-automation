using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.ResponseTypes.Element;

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

            InGenerics(
                "ElementGuids",
                "Elements to highlight.");

            InColors(
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

            InNumber(
                "Transparency",
                "Sets the transparency of the highlight (0.0: opaque, 1.0: transparent).",
                0.5);
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

            if (!ElementsObj.TryCreate(
                    da,
                    1,
                    out ElementsObj input) || input.Elements.Count == 0)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Warning,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            if (!da.TryGetItems(
                    2,
                    out List<GH_Colour> highlightedColors))
            {
                return;
            }


            if (!da.TryGetItem(
                    3,
                    out GH_Colour nonHighlightedColor))
            {
                return;
            }

            if (!da.TryGetItem(
                    4,
                    out bool wireframe3D))
            {
                return;
            }

            if (!da.TryGetItem(
                    5,
                    out double transparency))
            {
                return;
            }

            highlightedColors.MultiplyTo(input.Elements.Count);


            if (transparency < 0.0)
            {
                transparency = 0.0;
            }
            else if (transparency > 1.0)
            {
                transparency = 1.0;
            }

            // There is an error in the Archicad API implementation: the transparency
            // always comes from the non-highlighted element color.
            var highlightElements = new HighlightElementsObj()
            {
                Elements = input.Elements,
                HighlightedColors = Utilities.Convert.ToRgb(
                    highlightedColors,
                    255),
                NonHighlightedColor = Utilities.Convert.ToRgb(
                    nonHighlightedColor,
                    Convert.ToInt32(transparency * 255.0)),
                Wireframe3D = wireframe3D
            };

            SetArchiCadValues(
                CommandName,
                highlightElements);
        }

        private void ClearHighlight()
        {
            SetArchiCadValues(
                CommandName,
                new HighlightElementsObj());
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