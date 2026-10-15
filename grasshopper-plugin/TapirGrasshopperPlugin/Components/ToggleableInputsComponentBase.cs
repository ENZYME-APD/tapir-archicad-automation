using GH_IO.Serialization;
using Grasshopper.Kernel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Helps;

namespace TapirGrasshopperPlugin.Components
{
    // Shared base for the components whose optional inputs can be hidden and
    // shown again from the component's context menu, so a component with many
    // rarely used inputs does not have to occupy the whole canvas.
    //
    // The complete input layout is described once in InputDescriptors, in the
    // order the inputs are registered in. A hidden input has no parameter at
    // all, therefore the inputs must be addressed by name (IndexOfInput)
    // instead of by a fixed index everywhere.
    //
    // Every input is visible by default, so a document saved before the inputs
    // became toggleable - or one saved by a user who never opened the menu -
    // looks and behaves exactly as before. The names of the hidden inputs are
    // written into the document; IGH_VariableParameterComponent is implemented
    // (refusing every operation, the inputs are only toggled from the menu) to
    // tell Grasshopper that the parameter list of the component is not
    // necessarily the registered one.
    public abstract class ToggleableInputsComponentBase
        : ArchicadExecutorComponent, IGH_VariableParameterComponent
    {
        protected sealed class InputDescriptor
        {
            public InputDescriptor(
                string name,
                Func<IGH_Param> createParam,
                bool hideable)
            {
                Name = name;
                CreateParam = createParam;
                Hideable = hideable;
            }

            public string Name { get; }
            // Creates the parameter of the input. Called when the inputs are
            // registered and again every time the input is shown after hiding,
            // so it must always create an equivalent parameter.
            public Func<IGH_Param> CreateParam { get; }
            // Required inputs are never hideable.
            public bool Hideable { get; }
        }

        private readonly HashSet<string> _hiddenInputs = new HashSet<string>();

        // The complete input layout of the component in registration order.
        // Must not depend on instance state (see the note in the derived
        // bases): it is queried while the base constructor registers the
        // inputs.
        protected abstract IReadOnlyList<InputDescriptor> InputDescriptors { get; }

        protected ToggleableInputsComponentBase(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
        }

        protected sealed override void AddInputs()
        {
            foreach (var descriptor in InputDescriptors)
            {
                inManager.AddParameter(descriptor.CreateParam());
            }
        }

        // The index of an input in Params.Input, or -1 when it is hidden.
        protected int IndexOfInput(
            string name)
        {
            for (var index = 0; index < Params.Input.Count; index++)
            {
                if (Params.Input[index].Name == name)
                {
                    return index;
                }
            }

            return -1;
        }

        // Called after an input became visible again, so the derived class can
        // reattach whatever it attached to the input originally.
        protected virtual void OnInputShown(
            string name,
            int index)
        {
        }

        // Creates an input parameter equivalent to the ones the InX helpers of
        // Component register, so a re-shown input is identical to the one
        // registered at construction. A null typeName leaves the description
        // untouched, matching the helpers that do not prefix it either.
        protected static IGH_Param NewInputParam(
            IGH_Param param,
            string name,
            string description,
            string typeName,
            GH_ParamAccess access,
            bool optional)
        {
            param.Name = name;
            param.NickName = name;
            param.Description = typeName == null
                ? description
                : description.WithTypeName(typeName);
            param.Access = access;
            param.Optional = optional;
            return param;
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);

            var hideables = InputDescriptors.Where(x => x.Hideable).ToList();
            if (hideables.Count == 0)
            {
                return;
            }

            Menu_AppendSeparator(menu);

            var inputsMenu = Menu_AppendItem(
                menu,
                "Optional inputs");
            foreach (var descriptor in hideables)
            {
                var name = descriptor.Name;
                Menu_AppendItem(
                    inputsMenu.DropDown,
                    name,
                    (sender, e) => ToggleInput(name),
                    true,
                    !_hiddenInputs.Contains(name));
            }

            Menu_AppendSeparator(inputsMenu.DropDown);
            Menu_AppendItem(
                inputsMenu.DropDown,
                "Show all",
                (sender, e) => SetAllInputsHidden(false));
            Menu_AppendItem(
                inputsMenu.DropDown,
                "Hide all",
                (sender, e) => SetAllInputsHidden(true));
        }

        private void ToggleInput(
            string name)
        {
            RecordUndoEvent($"Toggle the {name} input");

            if (!_hiddenInputs.Remove(name))
            {
                _hiddenInputs.Add(name);
            }

            ApplyInputVisibility(true);
        }

        private void SetAllInputsHidden(
            bool hidden)
        {
            RecordUndoEvent(
                hidden
                    ? "Hide the optional inputs"
                    : "Show the optional inputs");

            _hiddenInputs.Clear();
            if (hidden)
            {
                foreach (var descriptor in InputDescriptors)
                {
                    if (descriptor.Hideable)
                    {
                        _hiddenInputs.Add(descriptor.Name);
                    }
                }
            }

            ApplyInputVisibility(true);
        }

        // Rebuilds Params.Input so that it holds exactly the not hidden inputs
        // in the order of InputDescriptors. The parameters that stay visible
        // are kept as they are, so their sources and stored values survive the
        // hiding of another input.
        private void ApplyInputVisibility(
            bool expireSolution)
        {
            var existing = new Dictionary<string, IGH_Param>();
            foreach (var param in Params.Input)
            {
                existing[param.Name] = param;
            }

            var visible = new List<IGH_Param>();
            var shownNames = new List<string>();
            foreach (var descriptor in InputDescriptors)
            {
                if (descriptor.Hideable &&
                    _hiddenInputs.Contains(descriptor.Name))
                {
                    continue;
                }

                if (existing.TryGetValue(
                        descriptor.Name,
                        out IGH_Param existingParam))
                {
                    visible.Add(existingParam);
                }
                else
                {
                    visible.Add(descriptor.CreateParam());
                    shownNames.Add(descriptor.Name);
                }
            }

            if (visible.Count == Params.Input.Count &&
                !visible.Where((
                    visibleParam,
                    position) => !ReferenceEquals(visibleParam, Params.Input[position])).Any())
            {
                return;
            }

            foreach (var param in Params.Input.ToList())
            {
                if (visible.Contains(param))
                {
                    continue;
                }

                param.RemoveAllSources();
                Params.UnregisterInputParameter(param);
            }

            // The kept parameters did not change their relative order, so every
            // position not holding the expected parameter is a newly shown one.
            for (var index = 0; index < visible.Count; index++)
            {
                if (index < Params.Input.Count &&
                    ReferenceEquals(Params.Input[index], visible[index]))
                {
                    continue;
                }

                Params.RegisterInputParam(
                    visible[index],
                    index);
            }

            Params.OnParametersChanged();

            // While the document is read back the component is not part of it
            // yet, and OnInputShown would have nothing to attach the sources to.
            if (OnPingDocument() != null)
            {
                foreach (var name in shownNames)
                {
                    OnInputShown(
                        name,
                        IndexOfInput(name));
                }
            }

            if (expireSolution)
            {
                ExpireSolution(true);
            }
        }

        public override bool Write(
            GH_IWriter writer)
        {
            if (!base.Write(writer))
            {
                return false;
            }

            writer.SetInt32(
                "HiddenInputCount",
                _hiddenInputs.Count);

            var index = 0;
            foreach (var name in _hiddenInputs)
            {
                writer.SetString(
                    "HiddenInput",
                    index,
                    name);
                index++;
            }

            return true;
        }

        public override bool Read(
            GH_IReader reader)
        {
            if (!base.Read(reader))
            {
                return false;
            }

            _hiddenInputs.Clear();

            var count = 0;
            if (reader.TryGetInt32(
                    "HiddenInputCount",
                    ref count))
            {
                for (var index = 0; index < count; index++)
                {
                    var name = string.Empty;
                    if (reader.TryGetString(
                            "HiddenInput",
                            index,
                            ref name))
                    {
                        _hiddenInputs.Add(name);
                    }
                }
            }
            else
            {
                // Documents written before the inputs became toggleable hold
                // every input, so nothing is hidden in them. Anything missing
                // from the restored parameters is taken as hidden anyway, so
                // the state stays consistent with what was read.
                foreach (var descriptor in InputDescriptors)
                {
                    if (descriptor.Hideable &&
                        IndexOfInput(descriptor.Name) < 0)
                    {
                        _hiddenInputs.Add(descriptor.Name);
                    }
                }
            }

            ApplyInputVisibility(false);

            return true;
        }

        // The inputs are toggled from the context menu only, the zoomable
        // insert and remove controls would not know which input to add.
        public bool CanInsertParameter(
            GH_ParameterSide side,
            int index)
        {
            return false;
        }

        public bool CanRemoveParameter(
            GH_ParameterSide side,
            int index)
        {
            return false;
        }

        public IGH_Param CreateParameter(
            GH_ParameterSide side,
            int index)
        {
            return null;
        }

        public bool DestroyParameter(
            GH_ParameterSide side,
            int index)
        {
            return false;
        }

        public void VariableParameterMaintenance()
        {
        }
    }
}
