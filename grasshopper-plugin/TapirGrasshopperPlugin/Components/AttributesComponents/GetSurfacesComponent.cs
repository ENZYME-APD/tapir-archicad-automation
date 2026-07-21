using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.AttributesComponents
{
    public class GetSurfacesComponent : AttributeStructuredGetterComponent
    {
        public override string CommandName => "GetSurfaces";

        public GetSurfacesComponent()
            : base(
                "GetSurfaces",
                "Get the details of the given Surface attributes.")
        {
        }

        protected override string ResponseArrayKey => "surfaces";

        private static readonly string[] NumberFields =
        {
            "ambientReflection", "diffuseReflection", "specularReflection",
            "transparency", "shine", "transparencyAttenuation",
            "emissionAttenuation"
        };

        private static readonly string[] TextureBooleanFields =
        {
            "FillRectangle", "FitPicture", "mirrorX", "mirrorY",
            "useAlphaChannel", "alphaChannelChangesTransparency",
            "alphaChannelChangesSurfaceColor", "alphaChannelChangesAmbientColor",
            "alphaChannelChangesSpecularColor", "alphaChannelChangesDiffuseColor"
        };

        protected override void AddOutputs()
        {
            OutGenerics(
                "AttributeGuids",
                "Identifier of each surface attribute.");

            OutIntegers(
                "Indices",
                "Index of each surface attribute.");

            OutTexts(
                "Names",
                "Name of each surface attribute.");

            OutTexts(
                "MaterialTypes",
                "Type of each surface material.");

            OutNumberList(
                "AmbientReflections",
                "Ambient reflection of each surface.");

            OutNumberList(
                "DiffuseReflections",
                "Diffuse reflection of each surface.");

            OutNumberList(
                "SpecularReflections",
                "Specular reflection of each surface.");

            OutNumberList(
                "Transparencies",
                "Transparency of each surface.");

            OutNumberList(
                "Shines",
                "Shininess of each surface.");

            OutNumberList(
                "TransparencyAttenuations",
                "Transparency attenuation of each surface.");

            OutNumberList(
                "EmissionAttenuations",
                "Emission attenuation of each surface.");

            OutColors(
                "SurfaceColors",
                "Surface color of each surface.");

            OutColors(
                "SpecularColors",
                "Specular color of each surface.");

            OutColors(
                "EmissionColors",
                "Emission color of each surface.");

            OutGenerics(
                "FillGuids",
                "Identifier of the fill attribute of each surface.");

            OutTexts(
                "TextureNames",
                "Name of the texture of each surface.");

            OutNumberList(
                "TextureRotationAngles",
                "Rotation angle of the texture of each surface.");

            OutNumberList(
                "TextureXSizes",
                "X size of the texture of each surface.");

            OutNumberList(
                "TextureYSizes",
                "Y size of the texture of each surface.");

            OutBooleans(
                "TextureFillRectangle",
                "True if the texture fills the rectangle.");

            OutBooleans(
                "TextureFitPicture",
                "True if the texture picture is fitted.");

            OutBooleans(
                "TextureMirrorX",
                "True if the texture is mirrored in X direction.");

            OutBooleans(
                "TextureMirrorY",
                "True if the texture is mirrored in Y direction.");

            OutBooleans(
                "TextureUseAlphaChannel",
                "True if the texture alpha channel is used.");

            OutBooleans(
                "TextureAlphaChangesTransparency",
                "True if the alpha channel changes the transparency.");

            OutBooleans(
                "TextureAlphaChangesSurfaceColor",
                "True if the alpha channel changes the surface color.");

            OutBooleans(
                "TextureAlphaChangesAmbientColor",
                "True if the alpha channel changes the ambient color.");

            OutBooleans(
                "TextureAlphaChangesSpecularColor",
                "True if the alpha channel changes the specular color.");

            OutBooleans(
                "TextureAlphaChangesDiffuseColor",
                "True if the alpha channel changes the diffuse color.");

            OutTexts(
                "ErrorMessages",
                "Error message for each queried attribute (empty when successful).");
        }

        protected override void SetOutputs(
            IGH_DataAccess da,
            List<JToken> items)
        {
            var guids = new List<object>();
            var indices = new List<object>();
            var names = new List<object>();
            var materialTypes = new List<object>();
            var numbers = new List<object>[NumberFields.Length];
            for (var f = 0; f < NumberFields.Length; f++)
            {
                numbers[f] = new List<object>();
            }
            var surfaceColors = new List<object>();
            var specularColors = new List<object>();
            var emissionColors = new List<object>();
            var fillGuids = new List<object>();
            var textureNames = new List<object>();
            var textureRotationAngles = new List<object>();
            var textureXSizes = new List<object>();
            var textureYSizes = new List<object>();
            var textureBooleans = new List<object>[TextureBooleanFields.Length];
            for (var f = 0; f < TextureBooleanFields.Length; f++)
            {
                textureBooleans[f] = new List<object>();
            }
            var errors = new List<string>();

            foreach (var item in items)
            {
                if (JsonOutputHelp.IsError(item))
                {
                    errors.Add(JsonOutputHelp.ErrorMessage(item));
                    guids.Add(null);
                    indices.Add(null);
                    names.Add(null);
                    materialTypes.Add(null);
                    foreach (var list in numbers)
                    {
                        list.Add(null);
                    }
                    surfaceColors.Add(null);
                    specularColors.Add(null);
                    emissionColors.Add(null);
                    fillGuids.Add(null);
                    textureNames.Add(null);
                    textureRotationAngles.Add(null);
                    textureXSizes.Add(null);
                    textureYSizes.Add(null);
                    foreach (var list in textureBooleans)
                    {
                        list.Add(null);
                    }
                    continue;
                }

                errors.Add("");
                guids.Add(AttributeIdOf(item));
                indices.Add(JsonOutputHelp.Scalar(item, "index"));
                names.Add(JsonOutputHelp.Scalar(item, "name"));
                materialTypes.Add(JsonOutputHelp.Scalar(item, "materialType"));
                for (var f = 0; f < NumberFields.Length; f++)
                {
                    numbers[f].Add(JsonOutputHelp.Scalar(item, NumberFields[f]));
                }
                surfaceColors.Add(JsonOutputHelp.ColorOf(item["surfaceColor"]));
                specularColors.Add(JsonOutputHelp.ColorOf(item["specularColor"]));
                emissionColors.Add(JsonOutputHelp.ColorOf(item["emissionColor"]));
                fillGuids.Add(WrappedAttributeIdOf(item["fillId"]));

                var texture = item["texture"];
                textureNames.Add(JsonOutputHelp.Scalar(texture, "name"));
                textureRotationAngles.Add(JsonOutputHelp.Scalar(texture, "rotationAngle"));
                textureXSizes.Add(JsonOutputHelp.Scalar(texture, "xSize"));
                textureYSizes.Add(JsonOutputHelp.Scalar(texture, "ySize"));
                for (var f = 0; f < TextureBooleanFields.Length; f++)
                {
                    textureBooleans[f].Add(
                        JsonOutputHelp.Scalar(texture, TextureBooleanFields[f]));
                }
            }

            da.SetDataList(0, guids);
            da.SetDataList(1, indices);
            da.SetDataList(2, names);
            da.SetDataList(3, materialTypes);
            for (var f = 0; f < NumberFields.Length; f++)
            {
                da.SetDataList(4 + f, numbers[f]);
            }
            da.SetDataList(11, surfaceColors);
            da.SetDataList(12, specularColors);
            da.SetDataList(13, emissionColors);
            da.SetDataList(14, fillGuids);
            da.SetDataList(15, textureNames);
            da.SetDataList(16, textureRotationAngles);
            da.SetDataList(17, textureXSizes);
            da.SetDataList(18, textureYSizes);
            for (var f = 0; f < TextureBooleanFields.Length; f++)
            {
                da.SetDataList(19 + f, textureBooleans[f]);
            }
            da.SetDataList(29, errors);
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetSurfaces;

        public override Guid ComponentGuid =>
            new Guid("1da2e5de-3f08-4592-88b3-4c72ee479414");
    }
}
