using Grasshopper;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.ElementsComponents
{
    public class GetTextureCoordinatesComponent
        : ElementsStructuredGetterComponent
    {
        public override string CommandName => "GetTextureCoordinates";

        public GetTextureCoordinatesComponent()
            : base(
                "GetTextureCoordinates",
                "Gets the texture coordinates (u, v) of the faces of the 3D model of the given elements: per " +
                "solid body, per face, at the first vertex of the face. The outputs have one branch per " +
                "element and body ({element;body}), with one item per face.",
                GroupNames.ElementDetails)
        {
        }

        protected override string ResponseArrayKey => "textureCoordinates";

        protected override void AddInputs()
        {
            InGenerics(
                "ElementGuids",
                "Identifiers of the elements. The bodies of a Stair, Railing or Curtain Wall, and of the " +
                "segments of a Column or Beam, belong to their subelements, which can be given instead.");
        }

        protected override void AddOutputs()
        {
            OutPointTree(
                "Points",
                "The first vertex of each face, in world coordinates (one branch per element and body; null " +
                "for a face without texture information).");

            OutGenericTree(
                "U",
                "The u texture coordinate at the point, in the same structure as Points.");

            OutGenericTree(
                "V",
                "The v texture coordinate at the point, in the same structure as Points.");

            OutErrorMessages(
                "Error message of each element (empty when successful).");

            OutTextTree(
                "BodyErrorMessages",
                "Error message of each body of each element (empty when successful; one branch per element).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var points = new DataTree<object>();
            var us = new DataTree<object>();
            var vs = new DataTree<object>();
            var errors = new List<string>();
            var bodyErrors = new DataTree<string>();

            for (var i = 0; i < items.Count; i++)
            {
                var item = items[i];
                var elementPath = new GH_Path(i);
                bodyErrors.EnsurePath(elementPath);

                if (JsonOutputHelp.IsError(item))
                {
                    points.EnsurePath(elementPath.AppendElement(0));
                    us.EnsurePath(elementPath.AppendElement(0));
                    vs.EnsurePath(elementPath.AppendElement(0));
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    continue;
                }

                errors.Add("");
                if (!(item["bodies"] is JArray bodies))
                {
                    continue;
                }

                for (var b = 0; b < bodies.Count; b++)
                {
                    var body = bodies[b];
                    var path = elementPath.AppendElement(b);
                    points.EnsurePath(path);
                    us.EnsurePath(path);
                    vs.EnsurePath(path);

                    if (JsonOutputHelp.IsError(body))
                    {
                        bodyErrors.Add(
                            JsonOutputHelp.ErrorMessage(body),
                            elementPath);
                        continue;
                    }

                    bodyErrors.Add(
                        "",
                        elementPath);
                    if (!(body["polygons"] is JArray polygons))
                    {
                        continue;
                    }

                    foreach (var polygon in polygons)
                    {
                        if (JsonOutputHelp.IsError(polygon))
                        {
                            points.Add(
                                null,
                                path);
                            us.Add(
                                null,
                                path);
                            vs.Add(
                                null,
                                path);
                            continue;
                        }

                        points.Add(
                            ElementGeometryJsonHelp.Point3D(polygon["point"]),
                            path);
                        us.Add(
                            JsonOutputHelp.Scalar(
                                polygon,
                                "u"),
                            path);
                        vs.Add(
                            JsonOutputHelp.Scalar(
                                polygon,
                                "v"),
                            path);
                    }
                }
            }

            da.SetDataTree(
                0,
                points);
            da.SetDataTree(
                1,
                us);
            da.SetDataTree(
                2,
                vs);
            da.SetDataList(
                3,
                errors);
            da.SetDataTree(
                4,
                bodyErrors);
        }

        // Shares the icon of GetBodyMaterialsOfElements until it gets one of its own.
        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetBodyMaterialsOfElements;

        public override Guid ComponentGuid =>
            new Guid("4f1d8644-21a1-46a7-b186-95174b4eac17");
    }
}
