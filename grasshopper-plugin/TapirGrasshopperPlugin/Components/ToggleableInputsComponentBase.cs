using GH_IO.Serialization;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Grasshopper.Kernel.Data;
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
    // all, therefore the inputs are addressed by name (IndexOfInput) instead
    // of by a fixed index everywhere. The name is the descriptor's, not the
    // parameter's: Grasshopper restores a parameter's Name from the document,
    // so an input renamed since the document was saved keeps its old Name.
    //
    // A freshly placed component starts with every hideable input hidden; only
    // the inputs that are not hideable (the required ones) are shown.
    //
    // Save format. This class deliberately does not implement
    // IGH_VariableParameterComponent. For such a component (decompiled
    // Grasshopper 7.21) GH_ComponentParamServer.Read calls
    // ReadAllParameterData, which starts with Clear() - removing every input
    // and output - and rebuilds them only from a "ParameterData" chunk. The
    // definitions saved before the inputs became toggleable have no such chunk
    // (GH_ComponentParamServer.Write writes "param_input"/"param_output"
    // chunks for every other component), so they would open with their
    // outputs and wires gone. Without the interface Grasshopper writes and
    // reads the inputs by index: Write writes one "param_input" chunk per
    // registered input, Read reads chunk i into Params.Input[i] for every
    // registered input and reports a missing chunk as a corrupt archive.
    //
    // Write therefore keeps exactly the layout of a component without hidden
    // inputs: it replaces the "param_input" chunks Grasshopper wrote for the
    // visible inputs with one chunk per descriptor, a hidden input being
    // written as an unconnected input with its default value. Only the
    // TapirInputCount and TapirHiddenInput items are added. A document saved
    // with hidden inputs thus still opens in the plugin versions without
    // toggleable inputs, with every input shown.
    //
    // Read registers one input per "param_input" chunk of the document, in
    // descriptor order, before Grasshopper reads them, so chunk i lands in the
    // input of descriptor i whichever inputs were hidden. Then it hides:
    // - with a TapirInputCount record (saved by this class): the inputs marked
    //   in TapirHiddenInput;
    // - without one (saved before the inputs became toggleable): every
    //   hideable input that has neither a wire nor stored data other than its
    //   default value, so no wire and no internalised value is lost;
    // - in both: the hideable inputs added after the document was saved (no
    //   chunk). An input with a wire or stored data is never hidden.
    // The undo records of Grasshopper (GH_GenericObjectAction) save and
    // restore the component through the same Write and Read.
    public abstract class ToggleableInputsComponentBase : ArchicadExecutorComponent
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
            // Creates the parameter of the input. Called when the input is
            // shown, when a document is read and when a hidden input is
            // saved, so it must always create an equivalent parameter.
            public Func<IGH_Param> CreateParam { get; }
            // Required inputs are never hideable.
            public bool Hideable { get; }
        }

        // The chunk name GH_ComponentParamServer reads and writes the inputs
        // with.
        private const string InputChunkName = "param_input";
        private const string InputCountKey = "TapirInputCount";
        private const string HiddenInputKey = "TapirHiddenInput";

        // Initialized before the base constructor runs, which registers the
        // inputs through AddInputs.
        private readonly HashSet<string> _hiddenInputs = new HashSet<string>(StringComparer.Ordinal);

        // The parameter of every visible input, by descriptor name.
        private readonly Dictionary<string, IGH_Param> _inputs = new Dictionary<string, IGH_Param>(StringComparer.Ordinal);

        // The complete input layout of the component in registration order.
        // Must not depend on instance state (see the note in the derived
        // bases): it is queried while the base constructor registers the
        // inputs. The names must be unique.
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
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var descriptor in InputDescriptors)
            {
                if (!names.Add(descriptor.Name))
                {
                    throw new InvalidOperationException(
                        $"The input {descriptor.Name} of {Name} is described twice.");
                }

                // A freshly placed component shows only the inputs that cannot
                // be hidden; Read restores the inputs of a saved component.
                if (descriptor.Hideable)
                {
                    _hiddenInputs.Add(descriptor.Name);
                    continue;
                }

                var param = descriptor.CreateParam();
                _inputs[descriptor.Name] = param;
                inManager.AddParameter(param);
            }
        }

        // The index of an input in Params.Input, or -1 when it is hidden.
        protected int IndexOfInput(
            string name)
        {
            return _inputs.TryGetValue(
                name,
                out IGH_Param param)
                ? Params.Input.IndexOf(param)
                : -1;
        }

        // Called after an input was shown from the context menu, so the
        // derived class can reattach whatever it attaches to a new input.
        protected virtual void OnInputShown(
            string name,
            int index)
        {
        }

        // Creates an input parameter equivalent to the ones the InX helpers of
        // Component register, so a re-shown input is identical to the one
        // registered at construction. A null typeName leaves the description
        // untouched, matching the inManager calls that do not prefix it either.
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
                (sender, e) => SetAllInputsHidden(false),
                hideables.Any(x => _hiddenInputs.Contains(x.Name)));
            Menu_AppendItem(
                inputsMenu.DropDown,
                "Hide all",
                (sender, e) => SetAllInputsHidden(true),
                hideables.Any(x => !_hiddenInputs.Contains(x.Name)));
        }

        private void ToggleInput(
            string name)
        {
            RecordUndoEvent(
                _hiddenInputs.Contains(name)
                    ? $"Show the {name} input"
                    : $"Hide the {name} input");

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
        // are kept as they are, so their wires and stored values survive the
        // hiding of another input. A hidden input loses its wires.
        // fromMenu: the change is the user's, so the solution is expired and
        // the inputs that were shown get their value lists.
        private void ApplyInputVisibility(
            bool fromMenu)
        {
            var visible = new List<IGH_Param>();
            var visibleByName = new Dictionary<string, IGH_Param>(StringComparer.Ordinal);
            var shownNames = new List<string>();
            foreach (var descriptor in InputDescriptors)
            {
                if (descriptor.Hideable &&
                    _hiddenInputs.Contains(descriptor.Name))
                {
                    continue;
                }

                if (!_inputs.TryGetValue(
                        descriptor.Name,
                        out IGH_Param param))
                {
                    param = descriptor.CreateParam();
                    shownNames.Add(descriptor.Name);
                }

                visible.Add(param);
                visibleByName[descriptor.Name] = param;
            }

            _inputs.Clear();
            foreach (var pair in visibleByName)
            {
                _inputs[pair.Key] = pair.Value;
            }

            if (visible.SequenceEqual(Params.Input))
            {
                return;
            }

            foreach (var param in Params.Input.ToList())
            {
                if (!visible.Contains(param))
                {
                    // Isolating the parameter removes its wires.
                    Params.UnregisterInputParameter(
                        param,
                        true);
                }
            }

            for (var index = 0; index < visible.Count; index++)
            {
                var param = visible[index];
                if (index < Params.Input.Count &&
                    ReferenceEquals(Params.Input[index], param))
                {
                    continue;
                }

                // Moved, not removed: its wires stay.
                Params.UnregisterInputParameter(
                    param,
                    false);
                Params.RegisterInputParam(
                    param,
                    index);
            }

            Params.OnParametersChanged();

            if (!fromMenu)
            {
                return;
            }

            foreach (var name in shownNames)
            {
                OnInputShown(
                    name,
                    IndexOfInput(name));
            }

            ExpireSolution(true);
        }

        public override bool Write(
            GH_IWriter writer)
        {
            var result = base.Write(writer);

            // GH_ComponentParamServer.Write wrote one chunk per visible input;
            // they are replaced by one chunk per descriptor (see the comment
            // at the top).
            for (var index = 0; index < Params.Input.Count; index++)
            {
                writer.RemoveChunk(
                    InputChunkName,
                    index);
            }

            var descriptors = InputDescriptors;
            writer.SetInt32(
                InputCountKey,
                descriptors.Count);
            for (var index = 0; index < descriptors.Count; index++)
            {
                var descriptor = descriptors[index];
                if (!_inputs.TryGetValue(
                        descriptor.Name,
                        out IGH_Param param))
                {
                    param = descriptor.CreateParam();
                    param.Attributes = new GH_LinkedParamAttributes(
                        param,
                        Attributes);
                    writer.SetBoolean(
                        HiddenInputKey,
                        index,
                        true);
                }

                result &= param.Write(
                    writer.CreateChunk(
                        InputChunkName,
                        index));
            }

            return result;
        }

        public override bool Read(
            GH_IReader reader)
        {
            var descriptors = InputDescriptors;

            // The inputs of the document, in descriptor order. A document saved
            // before an input was added has no chunk for it (nor for the inputs
            // after it, which were added later still).
            var archivedCount = 0;
            while (archivedCount < descriptors.Count &&
                   reader.ChunkExists(
                       InputChunkName,
                       archivedCount))
            {
                archivedCount++;
            }

            foreach (var param in Params.Input.ToList())
            {
                Params.UnregisterInputParameter(
                    param,
                    true);
            }

            _inputs.Clear();
            var archived = new IGH_Param[archivedCount];
            for (var index = 0; index < archivedCount; index++)
            {
                var param = descriptors[index].CreateParam();
                archived[index] = param;
                _inputs[descriptors[index].Name] = param;
                Params.RegisterInputParam(param);
            }

            // Reads chunk i into Params.Input[i], and the outputs.
            var result = base.Read(reader);

            var hasHiddenInputRecord = reader.ItemExists(InputCountKey);
            _hiddenInputs.Clear();
            for (var index = 0; index < descriptors.Count; index++)
            {
                var descriptor = descriptors[index];
                if (!descriptor.Hideable)
                {
                    continue;
                }

                if (index >= archivedCount)
                {
                    _hiddenInputs.Add(descriptor.Name);
                    continue;
                }

                if (IsInUse(
                        archived[index],
                        descriptor))
                {
                    continue;
                }

                var hidden = true;
                if (hasHiddenInputRecord)
                {
                    hidden = false;
                    reader.TryGetBoolean(
                        HiddenInputKey,
                        index,
                        ref hidden);
                }

                if (hidden)
                {
                    _hiddenInputs.Add(descriptor.Name);
                }
            }

            ApplyInputVisibility(false);

            // ApplyInputVisibility skips this when nothing had to move, but the
            // parameters were replaced above in any case.
            Params.OnParametersChanged();

            return result;
        }

        // Whether hiding the input would lose something: a wire, or stored
        // (internalised) data other than the value the input is created with.
        private static bool IsInUse(
            IGH_Param param,
            InputDescriptor descriptor)
        {
            if (param.SourceCount > 0)
            {
                return true;
            }

            return StoredData(param) != StoredData(descriptor.CreateParam());
        }

        // The persistent data of a GH_PersistentParam<T> as text, or an empty
        // string when it has none. There is no non-generic interface for it.
        private static string StoredData(
            IGH_Param param)
        {
            IGH_Structure data;
            try
            {
                data = param.GetType().GetProperty("PersistentData")?.GetValue(param) as IGH_Structure;
            }
            catch (Exception)
            {
                // Unknown: treated as stored data, so the input stays visible.
                return Guid.NewGuid().ToString();
            }

            return data == null || data.DataCount == 0
                ? string.Empty
                : data.DataDescription(
                    true,
                    true);
        }
    }
}
