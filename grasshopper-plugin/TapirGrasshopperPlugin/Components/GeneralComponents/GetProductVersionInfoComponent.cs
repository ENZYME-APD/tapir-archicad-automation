using Grasshopper.Kernel;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;

namespace TapirGrasshopperPlugin.Components.GeneralComponents
{
    public class GetProductVersionInfoComponent : ArchicadAccessorComponent
    {
        public override string CommandName => "GetProductVersionInfo";

        public GetProductVersionInfoComponent()
            : base(
                "GetProductVersionInfo",
                "Retrieves the version information of the running Archicad: build number, language code, product family " +
                "and version text, plus the product name, product type, main/update/hotfix versions, the available " +
                "domains (Architecture, MEP) and the CEIP project identifier of the open project from Archicad 29. " +
                "Requires Archicad 28 or later.",
                GroupNames.General)
        {
        }

        protected override void AddOutputs()
        {
            OutText(
                "ProductName",
                "The name of the product. Archicad 29 or later.");

            OutText(
                "ProductType",
                "The Graphisoft product type. Archicad 29 or later.");

            OutText(
                "ProductFamilyId",
                "The identifier of the product family.");

            OutText(
                "LanguageCode",
                "The Graphisoft language code of the running Archicad (for example INT, GER).");

            OutInteger(
                "BuildNumber",
                "The build number of the running Archicad.");

            OutText(
                "VersionString",
                "The full version of the running Archicad as text.");

            OutText(
                "MainVersion",
                "The main version, as text. Archicad 29 or later.");

            OutText(
                "UpdateVersion",
                "The update version, as text. Archicad 29 or later.");

            OutText(
                "HotfixVersion",
                "The hotfix version, as text. Archicad 29 or later.");

            OutIntegers(
                "VersionNumbers",
                "The three parts of the version number. Archicad 28 only.");

            OutTexts(
                "AvailableDomains",
                "The domains available in the running Archicad (Architecture, MEP). Archicad 29 or later.");

            OutText(
                "CeipProjectId",
                "The CEIP project identifier of the open project. Empty when no saved project is open. Archicad 29 or later.");
        }

        protected override void Solve(
            IGH_DataAccess da)
        {
            if (!TryGetCadResponse(
                    CommandName,
                    new JObject(),
                    ToAddOn,
                    out JObject response))
            {
                return;
            }

            da.SetData(
                0,
                response["productName"]?.ToString());

            da.SetData(
                1,
                response["productType"]?.ToString());

            da.SetData(
                2,
                response["productFamilyId"]?.ToString());

            da.SetData(
                3,
                response["languageCode"]?.ToString());

            da.SetData(
                4,
                (int?)response["buildNumber"]);

            da.SetData(
                5,
                response["versionString"]?.ToString());

            da.SetData(
                6,
                response["mainVersion"]?.ToString());

            da.SetData(
                7,
                response["updateVersion"]?.ToString());

            da.SetData(
                8,
                response["hotfixVersion"]?.ToString());

            da.SetDataList(
                9,
                response["versionNumbers"]?.ToObject<List<int>>() ??
                new List<int>());

            da.SetDataList(
                10,
                response["availableDomains"]?.ToObject<List<string>>() ??
                new List<string>());

            da.SetData(
                11,
                response["ceipProjectId"]?.ToString());
        }

        protected override System.Drawing.Bitmap Icon =>
            Properties.Resources.GetProductVersionInfo;

        public override Guid ComponentGuid =>
            new Guid("ab43b11c-9d14-4ad0-a2e3-2c69c409a391");
    }
}
