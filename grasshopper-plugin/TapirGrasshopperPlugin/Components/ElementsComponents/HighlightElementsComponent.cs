using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Data;
using System.Windows.Forms;
using System.Drawing;

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

            var inputElements = ElementsObj.Create(
                da,
                1);
            if (inputElements == null)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "Input ElementGuids failed to collect data.");
                return;
            }

            var highlightedColors = new List<GH_Colour>();
            if (!da.GetDataList(
                    2,
                    highlightedColors))
            {
                return;
            }

            if (highlightedColors.Count != 1 && inputElements.Elements.Count !=
                highlightedColors.Count)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    "The count of highlighted colors must be 1 or the same as the count of ElementGuids.");
                return;
            }

            var nonHighlightedColor = new GH_Colour();
            if (!da.GetData(
                    3,
                    ref nonHighlightedColor))
            {
                return;
            }

            var wireframe3D = false;
            if (!da.GetData(
                    4,
                    ref wireframe3D))
            {
                return;
            }

            var transparency = 0.0;
            if (!da.GetData<double>(
                    5,
                    ref transparency))
            {
                return;
            }

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
                Elements = inputElements.Elements,
                HighlightedColors = Utilities.Convert.ToRGBColors(
                    highlightedColors,
                    255,
                    inputElements.Elements.Count),
                NonHighlightedColor = Utilities.Convert.ToRGBColor(
                    nonHighlightedColor,
                    Convert.ToInt32(transparency * 255.0)),
                Wireframe3D = wireframe3D
            };

            GetResponse(
                CommandName,
                highlightElements);
        }

        private void ClearHighlight()
        {
            GetResponse(
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