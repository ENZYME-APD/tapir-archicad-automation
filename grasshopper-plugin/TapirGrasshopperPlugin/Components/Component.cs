using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Helps;
using TapirGrasshopperPlugin.Types.ArchiCad;

namespace TapirGrasshopperPlugin.Components
{
    public interface IButtonComponent
    {
        void OnCapsuleButtonPressed(
            int buttonIndex);

        List<string> CapsuleButtonTexts { get; set; }
    }

    public class ButtonAttributes : GH_ComponentAttributes
    {
        #region Fields

        private List<bool> _isPressed;
        private List<RectangleF> _buttonBounds;
        private int _additionalWidth;

        #endregion Fields

        #region Constructor

        public ButtonAttributes(
            IGH_Component component,
            int buttonCount = 1,
            int additionalWidth = 0)
            : base(component)
        {
            _isPressed = new List<bool>(new bool[buttonCount]);
            _buttonBounds = new List<RectangleF>(new RectangleF[buttonCount]);
            _additionalWidth = additionalWidth;
        }

        #endregion Constructor

        #region Methods

        protected override void Layout()
        {
            base.Layout();

            var componentCapsule = GH_Convert.ToRectangle(Bounds);
            componentCapsule.Width += _additionalWidth;
            componentCapsule.Height += _buttonBounds.Count * 30;

            for (var i = 0; i < _buttonBounds.Count; ++i)
            {
                var buttonCapsule = componentCapsule;
                buttonCapsule.Y = buttonCapsule.Bottom - ((i + 1) * 30);
                buttonCapsule.Height = 30;
                buttonCapsule.Inflate(
                    -5,
                    -5);
                _buttonBounds[i] = buttonCapsule;
            }

            Bounds = componentCapsule;
        }

        protected override void Render(
            GH_Canvas canvas,
            Graphics graphics,
            GH_CanvasChannel channel)
        {
            base.Render(
                canvas,
                graphics,
                channel);

            if (channel == GH_CanvasChannel.Objects)
            {
                if (Owner is IButtonComponent buttonComponent)
                {
                    for (var i = 0; i < _buttonBounds.Count; ++i)
                    {
                        var buttonCapsule = GH_Capsule.CreateTextCapsule(
                            _buttonBounds[i],
                            _buttonBounds[i],
                            _isPressed[i] ? GH_Palette.Grey : GH_Palette.Black,
                            buttonComponent.CapsuleButtonTexts[i],
                            5,
                            0);
                        buttonCapsule.Render(
                            graphics,
                            Selected,
                            Owner.Locked,
                            false);
                        buttonCapsule.Dispose();
                    }
                }
            }
        }

        public override GH_ObjectResponse RespondToMouseDown(
            GH_Canvas sender,
            GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left)
            {
                for (var i = 0; i < _buttonBounds.Count; ++i)
                {
                    if (_buttonBounds[i].Contains(e.CanvasLocation))
                    {
                        _isPressed[i] = true;
                        sender.Refresh();

                        return GH_ObjectResponse.Handled;
                    }
                }
            }

            return base.RespondToMouseDown(
                sender,
                e);
        }

        public override GH_ObjectResponse RespondToMouseUp(
            GH_Canvas sender,
            GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left)
            {
                for (var i = 0; i < _buttonBounds.Count; ++i)
                {
                    if (_buttonBounds[i].Contains(e.CanvasLocation) &&
                        _isPressed[i] == true)
                    {
                        if (Owner is IButtonComponent buttonComponent)
                        {
                            buttonComponent.OnCapsuleButtonPressed(i);
                            _isPressed[i] = false;
                            sender.Refresh();
                        }

                        return GH_ObjectResponse.Release;
                    }
                }
            }

            return base.RespondToMouseUp(
                sender,
                e);
        }

        #endregion Methods
    }

    public abstract class Component : GH_Component
    {
        protected static bool AutoRefresh = true;
        public bool ManualRefreshRequested = false;
        protected static bool AutoExecute = false;
        public bool ManualExecuteRequested = false;

        protected GH_InputParamManager inManager;
        protected GH_OutputParamManager outManager;

        protected override void RegisterInputParams(
            GH_InputParamManager pm)
        {
            inManager = pm;
            AddInputs();
        }

        protected override void RegisterOutputParams(
            GH_OutputParamManager pm)
        {
            outManager = pm;
            AddOutputs();
        }

        protected void AddAsSource<T>(
            GH_Document document,
            int index)
            where T : ValueList, new()
        {
            base.AddedToDocument(document);

            new T().AddAsSource(
                this,
                index);
        }

        protected virtual void AddInputs() { }
        protected virtual void AddOutputs() { }

        public Component(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                name,
                description,
                "Tapir",
                subCategory)
        {
        }

        public void SetOptionality(
            int[] indices)
        {
            foreach (var index in indices)
            {
                SetOptionality(index);
            }
        }

        protected void SetOptionality(
            int index)
        {
            inManager[index].Optional = true;
        }

        public void InGeneric(
            string name,
            string description = "")
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericItem"),
                GH_ParamAccess.item);
        }

        public void OutGeneric(
            string name,
            string description = "")
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericItem"),
                GH_ParamAccess.item);
        }

        public void OutGenerics(
            string name,
            string description = "")
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericList"),
                GH_ParamAccess.list);
        }

        public void InGenerics(
            string name,
            string description = "")
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericList"),
                GH_ParamAccess.list);
        }

        public void InGenericTree(
            string name,
            string description = "")
        {
            inManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericTree"),
                GH_ParamAccess.tree);
        }

        public void OutGenericTree(
            string name,
            string description = "")
        {
            outManager.AddGenericParameter(
                name,
                name,
                description.WithTypeName("genericTree"),
                GH_ParamAccess.tree);
        }

        public void InVectors(
            string name,
            string description = "")
        {
            inManager.AddVectorParameter(
                name,
                name,
                description.WithTypeName("vectorList"),
                GH_ParamAccess.list);
        }

        public void OutVectors(
            string name,
            string description = "")
        {
            outManager.AddVectorParameter(
                name,
                name,
                description.WithTypeName("vectorList"),
                GH_ParamAccess.list);
        }

        public void InPoints(
            string name,
            string description = "")
        {
            inManager.AddPointParameter(
                name,
                name,
                description.WithTypeName("pointList"),
                GH_ParamAccess.list);
        }

        public void OutPoints(
            string name,
            string description = "")
        {
            outManager.AddPointParameter(
                name,
                name,
                description.WithTypeName("pointList"),
                GH_ParamAccess.list);
        }

        public void InNumber(
            string name,
            string description,
            double defaultValue)
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void OutNumbers(
            string name,
            string description = "")
        {
            outManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberTree"),
                GH_ParamAccess.tree);
        }

        public void InNumbers(
            string name,
            string description = "")
        {
            inManager.AddNumberParameter(
                name,
                name,
                description.WithTypeName("numberList"),
                GH_ParamAccess.list);
        }

        public void InBoolean(
            string name,
            string description = "",
            bool defaultValue = false)
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InBooleans(
            string name,
            string description = "")
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanList"),
                GH_ParamAccess.list);
        }

        public void InBooleanTree(
            string name,
            string description = "")
        {
            inManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanTree"),
                GH_ParamAccess.tree);
        }

        public void OutBoolean(
            string name,
            string description = "")
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanItem"),
                GH_ParamAccess.item);
        }

        public void OutBooleans(
            string name,
            string description = "")
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanList"),
                GH_ParamAccess.list);
        }

        public void OutBooleanTree(
            string name,
            string description = "")
        {
            outManager.AddBooleanParameter(
                name,
                name,
                description.WithTypeName("booleanTree"),
                GH_ParamAccess.tree);
        }

        public void InInteger(
            string name,
            string description = "",
            int defaultValue = 0)
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InIntegers(
            string name,
            string description = "")
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerList"),
                GH_ParamAccess.list);
        }

        public void InIntegerTree(
            string name,
            string description = "")
        {
            inManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerTree"),
                GH_ParamAccess.tree);
        }

        public void OutInteger(
            string name,
            string description = "")
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerItem"),
                GH_ParamAccess.item);
        }

        public void OutIntegers(
            string name,
            string description = "")
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerList"),
                GH_ParamAccess.list);
        }

        public void OutIntegerTree(
            string name,
            string description = "")
        {
            outManager.AddIntegerParameter(
                name,
                name,
                description.WithTypeName("integerTree"),
                GH_ParamAccess.tree);
        }

        public void InText(
            string name,
            string description = "")
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textItem"),
                GH_ParamAccess.item);
        }

        public void InTextWithDefault(
            string name,
            string description = "",
            string defaultValue = "")
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void OutText(
            string name,
            string description = "")
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textItem"),
                GH_ParamAccess.item);
        }

        public void OutTexts(
            string name,
            string description = "")
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textList"),
                GH_ParamAccess.list);
        }

        public void OutTextTree(
            string name,
            string description = "")
        {
            outManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textTree"),
                GH_ParamAccess.tree);
        }

        public void InColor(
            string name,
            string description = "")
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorItem"),
                GH_ParamAccess.item);
        }

        public void InColor(
            string name,
            string description,
            Color defaultValue)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorItem"),
                GH_ParamAccess.item,
                defaultValue);
        }

        public void InColors(
            string name,
            string description,
            Color defaultValue)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorList"),
                GH_ParamAccess.list,
                defaultValue);
        }

        public void InColorTree(
            string name,
            string description,
            Color defaultValue)
        {
            inManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorTree"),
                GH_ParamAccess.tree);
        }

        public void OutColorTree(
            string name,
            string description = "")
        {
            outManager.AddColourParameter(
                name,
                name,
                description.WithTypeName("colorTree"),
                GH_ParamAccess.tree);
        }

        public void InTexts(
            string name,
            string description = "")
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textList"),
                GH_ParamAccess.list);
        }

        public void InTexts(
            string name,
            string defaultValue,
            string description = "")
        {
            inManager.AddTextParameter(
                name,
                name,
                description.WithTypeName("textList"),
                GH_ParamAccess.list,
                defaultValue);
        }

        public void OutBoxes(
            string name,
            string description = "")
        {
            outManager.AddBoxParameter(
                name,
                name,
                description.WithTypeName("boxList"),
                GH_ParamAccess.list);
        }

        public void OutCurves(
            string name,
            string description = "")
        {
            outManager.AddCurveParameter(
                name,
                name,
                description.WithTypeName("curveList"),
                GH_ParamAccess.list);
        }

        public void OutCurveTree(
            string name,
            string description = "")
        {
            outManager.AddCurveParameter(
                name,
                name,
                description.WithTypeName("curveTree"),
                GH_ParamAccess.tree);
        }

        protected CommandResponse ToArchicad(
            string commandName,
            JObject commandParameters)
        {
            return new ArchicadConnection(ConnectionSettings.Port).SendCommand(
                commandName,
                commandParameters);
        }

        protected CommandResponse ToAddOn(
            string commandName,
            JObject commandParameters)
        {
            return new ArchicadConnection(ConnectionSettings.Port)
                .SendAddOnCommand(
                    "TapirCommand",
                    commandName,
                    commandParameters);
        }

        protected bool TryGetCadResponse(
            string commandName,
            JObject commandParameters,
            Func<string, JObject, CommandResponse> sendCommand,
            out JObject response)
        {
            var cadResponse = sendCommand.Invoke(
                commandName,
                commandParameters);

            if (cadResponse.Succeeded)
            {
                response = cadResponse.Result;
                return true;
            }
            else
            {
                this.AddError(cadResponse.GetErrorMessage());

                response = null;
                return false;
            }
        }

        protected void SetCadValues(
            string commandName,
            object commandParameters,
            Func<string, JObject, CommandResponse> sendCommand)
        {
            TryGetCadResponse(
                commandName,
                JObject.FromObject(commandParameters),
                sendCommand,
                out var result);
        }

        protected bool TryGetConvertedCadValues<T>(
            string commandName,
            object commandParametersObject,
            Func<string, JObject, CommandResponse> sendCommand,
            Func<JObject, T> deserializer,
            out T response)
            where T : class
        {
            response = null;

            var commandParameters = commandParametersObject == null
                ? new JObject()
                : JObject.FromObject(commandParametersObject);

            if (!TryGetCadResponse(
                    commandName,
                    commandParameters,
                    sendCommand,
                    out var cadResponse))
            {
                return false;
            }

            try
            {
                response = deserializer.Invoke(cadResponse);
                return true;
            }
            catch (Exception ex)
            {
                this.AddError(
                    $"Failed to deserialize JSON response to {typeof(T).Name}: {ex.Message}");
            }

            return false;
        }
    }

    public abstract class ArchicadAccessorComponent
        : Component, IButtonComponent
    {
        public abstract string CommandName { get; }

        public ArchicadAccessorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { "Refresh" };
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(
                menu,
                "Refresh from Archicad",
                OnRefreshButtonClicked);
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(this);
        }

        public virtual void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            ManualRefresh();
        }

        private void OnRefreshButtonClicked(
            object sender,
            EventArgs e)
        {
            ManualRefresh();
        }

        public void ManualRefresh()
        {
            ManualRefreshRequested = true;
            try
            {
                ExpireSolution(true);
            }
            finally
            {
                ManualRefreshRequested = false;
            }
        }

        protected override void ExpireDownStreamObjects()
        {
            base.ExpireDownStreamObjects();

            foreach (var input in Params.Input)
            {
                if (input is ArchicadAccessorValueList valueList)
                {
                    valueList.RefreshItems();
                }
            }
        }

        protected override void SolveInstance(
            IGH_DataAccess da)
        {
            if (!AutoRefresh && !ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Outdated, waiting for manual refresh");
                return;
            }

            Solve(da);
        }

        public List<string> CapsuleButtonTexts { get; set; }

        protected abstract void Solve(
            IGH_DataAccess da);
    }

    public abstract class ArchicadExecutorComponent
        : Component, IButtonComponent
    {
        public abstract string CommandName { get; }

        public ArchicadExecutorComponent(
            string name,
            string description,
            string subCategory)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { "Execute" };
        }

        protected override void AppendAdditionalComponentMenuItems(
            ToolStripDropDown menu)
        {
            base.AppendAdditionalComponentMenuItems(menu);
            Menu_AppendItem(
                menu,
                "Execute in Archicad",
                OnExecuteButtonClicked);
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(this);
        }

        public virtual void OnCapsuleButtonPressed(
            int buttonIndex)
        {
            ManualExecute();
        }

        private void OnExecuteButtonClicked(
            object sender,
            EventArgs e)
        {
            ManualExecute();
        }

        public void ManualExecute()
        {
            ManualExecuteRequested = true;
            try
            {
                ExpireSolution(true);
            }
            finally
            {
                ManualExecuteRequested = false;
            }
        }

        protected override void ExpireDownStreamObjects()
        {
            base.ExpireDownStreamObjects();

            foreach (var input in Params.Input)
            {
                if (input is ArchicadAccessorValueList valueList)
                {
                    valueList.RefreshItems();
                }
            }
        }

        protected override void SolveInstance(
            IGH_DataAccess DA)
        {
            if (!AutoExecute && !ManualExecuteRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Waiting for manual execution");
                return;
            }

            Solve(DA);
        }

        protected abstract void Solve(
            IGH_DataAccess da);

        public List<string> CapsuleButtonTexts { get; set; }
    }

    public abstract class ButtonComponent : Component, IButtonComponent
    {
        private int _additionalWidth;

        public ButtonComponent(
            string name,
            string description,
            string subCategory,
            string buttonText,
            int additionalWidth = 0)
            : base(
                name,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string> { buttonText };
            _additionalWidth = additionalWidth;
            CreateAttributes();
        }

        public override void CreateAttributes()
        {
            m_attributes = new ButtonAttributes(
                this,
                2,
                _additionalWidth);
        }

        public abstract void OnCapsuleButtonPressed(
            int buttonIndex);

        public List<string> CapsuleButtonTexts { get; set; }
    }
}