using Grasshopper.Kernel;
using Grasshopper.Kernel.Parameters;
using System;
using System.Collections.Generic;
using System.Linq;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components.UtilitiesComponents
{
    public class DynamicMultiListFilterComponent
        : Component, IGH_VariableParameterComponent
    {
        public DynamicMultiListFilterComponent()
            : base(
                "DynamicMultiFilter",
                "Filters multiple lists in tandem",
                GroupNames.Utilities)
        {
        }

        private int NonDynamicInputCount => 2;

        private static string FieldNameByIndex(
            int index) =>
            $"List {index}";

        protected override void AddInputs()
        {
            InIntegers(
                "Indices",
                "Indices to extract items from the lists by.");

            InGenerics(FieldNameByIndex(0));
        }

        protected override void AddOutputs()
        {
            OutGeneric(FieldNameByIndex(0));
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!da.TryGetList(
                    0,
                    out List<int> indices))
            {
                return;
            }

            if (!da.TryGetList(
                    1,
                    out List<object> firstItems))
            {
                return;
            }

            da.SetDataList(
                0,
                Filter(
                    indices,
                    firstItems,
                    0));

            for (int i = 2; i < Params.Input.Count; i++)
            {
                if (!da.TryGetList(
                        i,
                        out List<object> dynamicItems))
                {
                    return;
                }

                da.SetDataList(
                    i - 1,
                    Filter(
                        indices,
                        dynamicItems,
                        i));
            }
        }

        private IEnumerable<object> Filter(
            List<int> indices,
            List<object> items,
            int index)
        {
            if (indices.Any(x => (items.Count - 1) < x || x < 0))
            {
                this.AddError(
                    $"Invalid index(es) at the {index}th input field!");
            }

            return items.Where((
                item,
                i) => indices.Contains(i));
        }

        public bool CanInsertParameter(
            GH_ParameterSide side,
            int index)
        {
            return side == GH_ParameterSide.Input &&
                   NonDynamicInputCount <= index;
        }

        public bool CanRemoveParameter(
            GH_ParameterSide side,
            int index)
        {
            return side == GH_ParameterSide.Input &&
                   NonDynamicInputCount <= index &&
                   index == Params.Input.Count - 1;
        }

        public IGH_Param CreateParameter(
            GH_ParameterSide side,
            int index)
        {
            var newInput = new Param_GenericObject();
            var newOutput = new Param_GenericObject();

            var name = FieldNameByIndex(index - 1);

            newInput.Name = name;
            newInput.NickName = name;
            newInput.Description = name;
            newInput.Access = GH_ParamAccess.list;

            newOutput.Name = name;
            newOutput.NickName = name;
            newOutput.Description = name;
            newOutput.Access = GH_ParamAccess.list;

            Params.Input.Insert(
                index,
                newInput);

            Params.Output.Insert(
                index - 1,
                newOutput);

            VariableParameterMaintenance();

            return newInput;
        }

        public bool DestroyParameter(
            GH_ParameterSide side,
            int index)
        {
            if (side == GH_ParameterSide.Input && NonDynamicInputCount <= index)
            {
                Params.Input.RemoveAt(index);
                Params.Output.RemoveAt(index - 1);

                VariableParameterMaintenance();
                return true;
            }

            return false;
        }

        public void VariableParameterMaintenance()
        {
            for (int i = NonDynamicInputCount; i < Params.Input.Count; i++)
            {
                var name = FieldNameByIndex(i - 1);
                Params.Input[i].Name = name;
                Params.Input[i].NickName = name;
            }

            for (int i = NonDynamicInputCount - 1; i < Params.Output.Count; i++)
            {
                var name = FieldNameByIndex(i);
                Params.Output[i].Name = name;
                Params.Output[i].NickName = name;
            }

            ExpireSolution(true);
        }

        public override Guid ComponentGuid =>
            new Guid("aa37107d-57bd-415d-bbf6-0b36058451e3");
    }
}