using Grasshopper.Kernel;
using Grasshopper.Kernel.Parameters;
using Grasshopper.Kernel.Types;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.Generic;
using TapirGrasshopperPlugin.Types.GuidObjects;
using TapirGrasshopperPlugin.Types.Project;

namespace TapirGrasshopperPlugin.Components.ProjectComponents
{
    // Shared by the project, project info group and project data components.
    internal static class ProjectComponentHelp
    {
        // Optional inputs without a default value: an unconnected one gives
        // nothing, so the components leave its key out of the command and
        // Archicad keeps the current value. Registered through Params, as the
        // parameter managers of GH_Component are protected.
        private static void AddOptionalInput(
            GH_Component component,
            IGH_Param param,
            string name,
            string description,
            string typeName)
        {
            param.Name = name;
            param.NickName = name;
            param.Description = description.WithTypeName(typeName);
            param.Access = GH_ParamAccess.item;
            param.Optional = true;
            component.Params.RegisterInputParam(param);
        }

        public static void AddOptionalText(
            this GH_Component component,
            string name,
            string description)
        {
            AddOptionalInput(
                component,
                new Param_String(),
                name,
                description,
                "textItem");
        }

        public static void AddOptionalInteger(
            this GH_Component component,
            string name,
            string description)
        {
            AddOptionalInput(
                component,
                new Param_Integer(),
                name,
                description,
                "integerItem");
        }

        public static void AddOptionalNumber(
            this GH_Component component,
            string name,
            string description)
        {
            AddOptionalInput(
                component,
                new Param_Number(),
                name,
                description,
                "numberItem");
        }

        public static void AddOptionalBoolean(
            this GH_Component component,
            string name,
            string description)
        {
            AddOptionalInput(
                component,
                new Param_Boolean(),
                name,
                description,
                "booleanItem");
        }

        private static void AddItemOutput(
            GH_Component component,
            IGH_Param param,
            string name,
            string description,
            string typeName)
        {
            param.Name = name;
            param.NickName = name;
            param.Description = description.WithTypeName(typeName);
            param.Access = GH_ParamAccess.item;
            component.Params.RegisterOutputParam(param);
        }

        public static void AddNumberOutput(
            this GH_Component component,
            string name,
            string description)
        {
            AddItemOutput(
                component,
                new Param_Number(),
                name,
                description,
                "numberItem");
        }

        public static void AddColorOutput(
            this GH_Component component,
            string name,
            string description)
        {
            AddItemOutput(
                component,
                new Param_Colour(),
                name,
                description,
                "colorItem");
        }

        // Adds the value of an optional input to the command parameters under
        // the given key, when the input has one.
        public static bool TryAddOptional<T>(
            this IGH_DataAccess da,
            int index,
            JObject parameters,
            string key)
        {
            if (!da.TryGet(
                    index,
                    out T value) ||
                value == null)
            {
                return false;
            }

            parameters[key] = JToken.FromObject(value);
            return true;
        }

        // The message of an ExecutionResult object ({"success": ...}).
        public static string ExecutionResultMessage(
            JToken executionResult)
        {
            return executionResult is JObject result
                ? ExecutionResult.Deserialize(result).Message()
                : "No execution result.";
        }

        // The Project Info group identifiers of a generic input: the outputs
        // of GetProjectInfoGroups or CreateProjectInfoGroups, or guid texts.
        public static bool TryGetProjectInfoGroupIds(
            this GH_ActiveObject component,
            List<GH_ObjectWrapper> wrappers,
            string inputName,
            out List<ProjectInfoGroupGuidWrapper> ids)
        {
            ids = new List<ProjectInfoGroupGuidWrapper>();
            foreach (var wrapper in wrappers)
            {
                var id = wrapper == null
                    ? null
                    : GuidWrapper<ProjectInfoGroupGuid, ProjectInfoGroupGuidWrapper>
                        .CreateFromGhWrapper(wrapper);
                if (id == null)
                {
                    component.AddError(
                        $"Invalid Project Info group identifier in the {inputName} input.");
                    return false;
                }

                ids.Add(id);
            }

            return true;
        }

        // The per-item values of an input given once for all items or once per
        // item; null when the input has a different number of values.
        public static List<T> ExpandToCount<T>(
            this GH_ActiveObject component,
            List<T> values,
            int count,
            string inputName,
            string countInputName)
        {
            if (values.Count == count)
            {
                return values;
            }

            if (values.Count == 1)
            {
                var expanded = new List<T>();
                for (var i = 0; i < count; i++)
                {
                    expanded.Add(values[0]);
                }

                return expanded;
            }

            component.AddError(
                $"The size of the input {inputName} must be 1 or equal to the size of the input {countInputName}.");
            return null;
        }
    }
}
