using Grasshopper.Kernel;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.GuidObjects;

namespace TapirGrasshopperPlugin.Types.Analytical
{
    public class AnalyticalModelGuid : GuidObject<AnalyticalModelGuid>
    {
    }

    public class LoadGroupGuid : GuidObject<LoadGroupGuid>
    {
    }

    public class LoadGroupGuidWrapper
        : GuidWrapper<LoadGroupGuid, LoadGroupGuidWrapper>
    {
        [JsonProperty("loadGroupId")]
        public LoadGroupGuid LoadGroupId;

        [JsonIgnore]
        public override LoadGroupGuid Id
        {
            get => LoadGroupId;
            set => LoadGroupId = value;
        }
    }

    public class LoadGroupIdsObject
        : GuidItemsObject<LoadGroupGuid, LoadGroupGuidWrapper, LoadGroupIdsObject>
    {
        [JsonProperty("loadGroupIds")]
        public List<LoadGroupGuidWrapper> LoadGroupIds;

        [JsonIgnore]
        public override List<LoadGroupGuidWrapper> GuidWrappers
        {
            get => LoadGroupIds;
            set => LoadGroupIds = value;
        }
    }

    public class LoadCaseGuid : GuidObject<LoadCaseGuid>
    {
    }

    public class LoadCaseGuidWrapper
        : GuidWrapper<LoadCaseGuid, LoadCaseGuidWrapper>
    {
        [JsonProperty("loadCaseId")]
        public LoadCaseGuid LoadCaseId;

        [JsonIgnore]
        public override LoadCaseGuid Id
        {
            get => LoadCaseId;
            set => LoadCaseId = value;
        }
    }

    public class LoadCaseIdsObject
        : GuidItemsObject<LoadCaseGuid, LoadCaseGuidWrapper, LoadCaseIdsObject>
    {
        [JsonProperty("loadCaseIds")]
        public List<LoadCaseGuidWrapper> LoadCaseIds;

        [JsonIgnore]
        public override List<LoadCaseGuidWrapper> GuidWrappers
        {
            get => LoadCaseIds;
            set => LoadCaseIds = value;
        }
    }

    public class LoadCombinationGuid : GuidObject<LoadCombinationGuid>
    {
    }

    public class LoadCombinationGuidWrapper
        : GuidWrapper<LoadCombinationGuid, LoadCombinationGuidWrapper>
    {
        [JsonProperty("loadCombinationId")]
        public LoadCombinationGuid LoadCombinationId;

        [JsonIgnore]
        public override LoadCombinationGuid Id
        {
            get => LoadCombinationId;
            set => LoadCombinationId = value;
        }
    }

    public class LoadCombinationIdsObject
        : GuidItemsObject<LoadCombinationGuid, LoadCombinationGuidWrapper, LoadCombinationIdsObject>
    {
        [JsonProperty("loadCombinationIds")]
        public List<LoadCombinationGuidWrapper> LoadCombinationIds;

        [JsonIgnore]
        public override List<LoadCombinationGuidWrapper> GuidWrappers
        {
            get => LoadCombinationIds;
            set => LoadCombinationIds = value;
        }
    }

    // The fields of the Structural Analytical Model visibility settings, shared
    // by GetAnalyticalVisibilitySettings and SetAnalyticalVisibilitySettings.
    public static class AnalyticalVisibilitySettings
    {
        // The boolean fields with the names of their parameters, in the order
        // of the parameters after VisibilityMode.
        public static readonly (string Key, string Name, string Description)[] Flags =
        {
            (
                "isAnalyticalModelVisible",
                "IsAnalyticalModelVisible",
                "Whether the Structural Analytical Model is shown."),
            (
                "areMembersTransparent",
                "AreMembersTransparent",
                "Whether the analytical members are transparent."),
            (
                "areSymbolsTransparent",
                "AreSymbolsTransparent",
                "Whether the symbols are transparent."),
            (
                "areNodesVisible",
                "AreNodesVisible",
                "Whether the nodes are shown."),
            (
                "areReleasesVisible",
                "AreReleasesVisible",
                "Whether the releases are shown."),
            (
                "areCrossSectionsVisible",
                "AreCrossSectionsVisible",
                "Whether the cross sections are shown."),
            (
                "areCoordinationProfilesVisible",
                "AreCoordinationProfilesVisible",
                "Whether the coordination profiles are shown."),
            (
                "areLocalCoordSystemsVisible",
                "AreLocalCoordSystemsVisible",
                "Whether the local coordinate systems are shown.")
        };

        public const string VisibilityModeDoc =
            "Whether only the analytical model is shown or the analytical model within the entire model: " +
            "AnalyticalModelOnly or AnalyticalModelWithinEntireModel.";

        public const string LoadVisibilityModeDoc =
            "Which loads are shown: FilterByActive, HideAll or ShowAll.";
    }

    // The values the structural analysis load commands accept and return,
    // for the descriptions of the parameters.
    public static class AnalyticalLoadEnums
    {
        public const string GroupTypes =
            "PermanentLoad, VariableLoad, AccidentalLoad, SeismicLoad, MovingLoad, TensioningLoad or FireLoad";

        public const string Relations =
            "ExclusiveRelation, StandardRelation or TogetherRelation";

        public const string ActionTypes =
            "PermanentAction, VariableAction or AccidentalAction";

        public const string LoadCaseLoadTypes =
            "SelfWeightLoad, PrestressLoad, DynamicLoad, StaticLoad, TemperatureLoad, WindLoad, SnowLoad, " +
            "MaintenanceLoad, FireLoad, MovingLoad, SeismicLoad, StandardLoad or OthersLoad";

        public const string Durations =
            "LongDuration, MediumDuration, ShortDuration or InstantaneousDuration";

        public const string Categories =
            "ULS, SLS, ALS, ANS or NotDefined";

        public const string NationalStandards =
            "ENULSSTRGEOSetB, ENULSSTRGEOSetC, ENAccidental1, ENAccidental2, ENSeismic, ENSLSCharacteristic, " +
            "ENSLSFrequent, ENSLSQuasipermanent, IBCLRFDultimate, IBCASDultimate, IBCASDserviceability, " +
            "IBCASDseismic or IBCLRFDseismic";

        public const string CombinationTypes =
            "Envelope, Linear or Nonlinear";
    }

    // Helpers shared by the Structural Analytical Model components.
    public static class AnalyticalHelp
    {
        // An identifier object like {"guid": "..."} of the given guid object.
        public static JObject IdOf<T>(
            T id)
            where T : GuidObject<T>, new()
        {
            return new JObject { ["guid"] = id.Guid };
        }

        // The typed identifier in an identifier object like {"guid": "..."};
        // null when it is absent.
        public static T IdFrom<T>(
            JToken idObject)
            where T : GuidObject<T>, new()
        {
            var guid = JsonOutputHelp.GuidOf(idObject);
            return guid == null ? null : new T { Guid = guid };
        }

        // Reads an optional input of typed identifiers. Returns false (after
        // reporting an error) when one of them is not a valid identifier.
        public static bool TryGetIds<T>(
            GH_ActiveObject component,
            IGH_DataAccess da,
            int index,
            string inputName,
            out List<T> ids)
            where T : GuidObject<T>, new()
        {
            ids = new List<T>();
            da.TryGetList(
                index,
                out List<GH_ObjectWrapper> wrappers);
            foreach (var wrapper in wrappers ?? new List<GH_ObjectWrapper>())
            {
                var id = GuidObject<T>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    component.AddError($"Invalid identifier in the {inputName} input.");
                    return false;
                }
                ids.Add(id);
            }
            return true;
        }

        // Checks that an optional per-item input has 0, 1 or count items.
        public static bool CheckOptionalCount(
            GH_ActiveObject component,
            int inputCount,
            int count,
            string inputName,
            string countName)
        {
            if (inputCount > 1 &&
                inputCount != count)
            {
                component.AddError(
                    $"The size of the input {inputName} must be 0, 1 or equal to the size of the input {countName}.");
                return false;
            }
            return true;
        }

        // Checks that a required per-item input has 1 or count items.
        public static bool CheckRequiredCount(
            GH_ActiveObject component,
            int inputCount,
            int count,
            string inputName,
            string countName)
        {
            if (inputCount != 1 &&
                inputCount != count)
            {
                component.AddError(
                    $"The size of the input {inputName} must be 1 or equal to the size of the input {countName}.");
                return false;
            }
            return true;
        }

        // The item of a per-item input for the given index: the only item
        // when there is one, the item at the index otherwise.
        public static T ItemFor<T>(
            List<T> items,
            int index)
        {
            return items[items.Count == 1 ? 0 : index];
        }

        // The typed identifiers of a list of Grasshopper items. Returns false
        // (after reporting an error) when one of them is not a valid identifier.
        public static bool TryConvertIds<T>(
            GH_ActiveObject component,
            IEnumerable<IGH_Goo> items,
            string inputName,
            out List<T> ids)
            where T : GuidObject<T>, new()
        {
            ids = new List<T>();
            foreach (var goo in items)
            {
                var wrapper = goo as GH_ObjectWrapper ?? new GH_ObjectWrapper(goo);
                var id = GuidObject<T>.CreateFromWrapper(wrapper);
                if (id == null)
                {
                    component.AddError($"Invalid identifier in the {inputName} input.");
                    return false;
                }
                ids.Add(id);
            }
            return true;
        }

        // Builds the loadFactors of a load combination from its load cases.
        // Factors and Multipliers hold 0, 1 (the same for all) or one item per
        // load case; a factor left out is not set, a multiplier left out is 1.
        public static bool TryBuildLoadFactors(
            GH_ActiveObject component,
            List<LoadCaseGuid> loadCaseIds,
            List<double> factors,
            List<double> multipliers,
            out JArray loadFactors)
        {
            loadFactors = new JArray();
            if (!CheckOptionalCount(component, factors.Count, loadCaseIds.Count, "Factors", "LoadCaseGuids") ||
                !CheckOptionalCount(component, multipliers.Count, loadCaseIds.Count, "Multipliers", "LoadCaseGuids"))
            {
                return false;
            }

            for (var i = 0; i < loadCaseIds.Count; i++)
            {
                var loadFactor = new JObject
                {
                    ["loadCaseId"] = IdOf(loadCaseIds[i]),
                    ["multiplier"] = multipliers.Count > 0 ? ItemFor(multipliers, i) : 1.0
                };
                if (factors.Count > 0)
                {
                    loadFactor["factor"] = ItemFor(factors, i);
                }
                loadFactors.Add(loadFactor);
            }
            return true;
        }

        // Turns the outcome of a command that answers with a single
        // ExecutionResult into an error message on the component.
        public static void ReportExecutionResult(
            GH_ActiveObject component,
            JObject response,
            string failureMessage)
        {
            if ((bool?)response?["success"] != true)
            {
                component.AddError(
                    response?["error"]?["message"]?.ToString() ??
                    failureMessage);
            }
        }
    }
}
