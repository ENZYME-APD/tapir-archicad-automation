using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Element;

namespace TapirGrasshopperPlugin.Components.IFCComponents
{
    public class GetIFCClassificationReferencesOfElementsComponent
        : ArchicadAccessorComponent
    {
        public override string CommandName =>
            "GetIFCClassificationReferencesOfElements";

        public GetIFCClassificationReferencesOfElementsComponent()
            : base(
                "GetIFCClassificationReferencesOfElements",
                "Retrieve the IFC classification references of the given elements, subelements included: " +
                "identification, name, location, description (from Archicad 29), the referenced classification " +
                "(name, source, edition) and whether the reference comes from the preview translator or is stored " +
                "locally. On Archicad 25, 26 and 27 the source is not given, the referenced classification also has " +
                "its edition date, description and location, and StoredOnly returns only the references stored on " +
                "the elements. Reference outputs have one branch per element.",
                GroupNames.IFC)
        {
        }

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements to query.");

            InBoolean(
                "StoredOnly",
                "Return only the classification references stored on the elements, not the ones Archicad would " +
                "generate on IFC export. Used only in Archicad 25, 26 and 27. Default is false.");

            SetOptionality(1);
        }

        protected override void AddOutputs()
        {
            OutGenerics(
                "ElementGuids",
                "Identifier of each queried element.");

            OutTextTree(
                "Identifications",
                "Identification of each classification reference, for example a classification code.");

            OutTextTree(
                "Names",
                "Name of each classification reference.");

            OutTextTree(
                "Locations",
                "Location of each classification reference, for example a URL.");

            OutTextTree(
                "Descriptions",
                "Description of each classification reference (from Archicad 29).");

            OutTextTree(
                "ClassificationNames",
                "Name of the classification (IfcClassification) each reference belongs to.");

            OutTextTree(
                "ClassificationSources",
                "Source of the classification each reference belongs to.");

            OutTextTree(
                "ClassificationEditions",
                "Edition of the classification each reference belongs to.");

            OutTextTree(
                "ClassificationEditionDates",
                "Edition date of the classification (Archicad 25, 26 and 27 only).");

            OutTextTree(
                "ClassificationDescriptions",
                "Description of the classification (Archicad 25, 26 and 27 only).");

            OutTextTree(
                "ClassificationLocations",
                "Location of the classification (Archicad 25, 26 and 27 only).");

            OutTextTree(
                "Sources",
                "Where each reference comes from: preview (mapped by the preview IFC export translator) or " +
                "local (stored on the element). Not given in Archicad 25, 26 and 27.");

            OutErrorMessages(
                "Error message for each queried element (empty when successful).");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!da.TryCreateFromList(
                    0,
                    out ElementsObject elements))
            {
                return;
            }

            var parameters = JObject.FromObject(elements);

            da.TryGet(
                1,
                out bool storedOnly);
            if (storedOnly)
            {
                parameters["storedOnly"] = true;
            }

            if (!TryGetCadResponse(
                    CommandName,
                    parameters,
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            // Output index and where its value is: on the reference itself or
            // on its referencedSource.
            var fields = new (int Output, bool OfSource, string Key)[]
            {
                (1, false, "identification"),
                (2, false, "name"),
                (3, false, "location"),
                (4, false, "description"),
                (5, true, "name"),
                (6, true, "source"),
                (7, true, "edition"),
                (8, true, "editionDate"),
                (9, true, "description"),
                (10, true, "location"),
                (11, false, "source")
            };

            var elementGuids = new List<object>();
            var trees = new DataTree<object>[fields.Length];
            for (var f = 0; f < fields.Length; f++)
            {
                trees[f] = new DataTree<object>();
            }
            var errors = new List<string>();

            var items = JsonOutputHelp.Items(
                response,
                "elementIFCClassificationReferences");
            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var path = new GH_Path(i);
                foreach (var tree in trees)
                {
                    tree.EnsurePath(path);
                }

                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    elementGuids.Add(null);
                    continue;
                }

                errors.Add("");
                elementGuids.Add(ElementsStructuredGetterComponent.ElementIdOf(item));

                if (!(item["ifcClassificationReferences"] is JArray references))
                {
                    continue;
                }

                foreach (var reference in references)
                {
                    for (var f = 0; f < fields.Length; f++)
                    {
                        var owner = fields[f].OfSource
                            ? reference["referencedSource"]
                            : reference;
                        trees[f].Add(
                            JsonOutputHelp.Scalar(
                                owner,
                                fields[f].Key),
                            path);
                    }
                }
            }

            da.SetDataList(0, elementGuids);
            for (var f = 0; f < fields.Length; f++)
            {
                da.SetDataTree(fields[f].Output, trees[f]);
            }
            da.SetDataList(12, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetIFCClassificationReferencesOfElements;

        public override Guid ComponentGuid =>
            new Guid("3b971784-1a17-454d-96fa-d013144f4b92");
    }
}
