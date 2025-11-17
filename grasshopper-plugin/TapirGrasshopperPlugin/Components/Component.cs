using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using TapirGrasshopperPlugin.Utilities;

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

            var componentCapsule = GH_Convert.ToRectangle(this.Bounds);
            componentCapsule.Width += _additionalWidth;
            componentCapsule.Height += this._buttonBounds.Count * 30;

            for (var i = 0; i < this._buttonBounds.Count; ++i)
            {
                var buttonCapsule = componentCapsule;
                buttonCapsule.Y = buttonCapsule.Bottom - (i + 1) * 30;
                buttonCapsule.Height = 30;
                buttonCapsule.Inflate(
                    -5,
                    -5);
                this._buttonBounds[i] = buttonCapsule;
            }

            this.Bounds = componentCapsule;
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
                if (this.Owner is IButtonComponent buttonComponent)
                {
                    for (var i = 0; i < this._buttonBounds.Count; ++i)
                    {
                        var buttonCapsule = GH_Capsule.CreateTextCapsule(
                            box: this._buttonBounds[i],
                            textbox: this._buttonBounds[i],
                            palette: this._isPressed[i]
                                ? GH_Palette.Grey
                                : GH_Palette.Black,
                            text: buttonComponent.CapsuleButtonTexts[i],
                            radius: 5,
                            highlight: 0);
                        buttonCapsule.Render(
                            graphics,
                            this.Selected,
                            this.Owner.Locked,
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
                for (var i = 0; i < this._buttonBounds.Count; ++i)
                {
                    if (this._buttonBounds[i].Contains(e.CanvasLocation))
                    {
                        this._isPressed[i] = true;
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
            if (e.Button == System.Windows.Forms.MouseButtons.Left)
            {
                for (var i = 0; i < this._buttonBounds.Count; ++i)
                {
                    if (this._buttonBounds[i].Contains(e.CanvasLocation) &&
                        this._isPressed[i] == true)
                    {
                        if (this.Owner is IButtonComponent buttonComponent)
                        {
                            buttonComponent.OnCapsuleButtonPressed(i);
                            this._isPressed[i] = false;
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

    abstract public class Component : GH_Component
    {
        protected static bool AutoRefresh = true;
        public bool ManualRefreshRequested = false;
        protected static bool AutoExecute = false;
        public bool ManualExecuteRequested = false;

        private static string CommandNameSpace => "TapirCommand";

        public Component(
            string name,
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
                description,
                "Tapir",
                subCategory)
        {
        }

        protected CommandResponse SendArchicadCommand(
            string commandName,
            JObject commandParameters)
        {
            var connection = new ArchicadConnection(ConnectionSettings.Port);
            return connection.SendCommand(
                commandName,
                commandParameters);
        }

        protected CommandResponse SendArchicadAddOnCommand(
            string commandName,
            JObject commandParameters)
        {
            var connection = new ArchicadConnection(ConnectionSettings.Port);

            return connection.SendAddOnCommand(
                CommandNameSpace,
                commandName,
                commandParameters);
        }

        protected bool GetArchicadAddonResponse(
            string commandName,
            JObject commandParameters,
            out JObject response)
        {
            var cResponse = SendArchicadAddOnCommand(
                commandName,
                commandParameters);

            if (!cResponse.Succeeded)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Error,
                    cResponse.GetErrorMessage());
                response = null;
                return false;
            }

            response = cResponse.Result;
            return true;
        }

        protected bool GetResponse<T>(
            string commandName,
            JObject commandParameters,
            out T response)
            where T : class
        {
            if (!GetArchicadAddonResponse(
                    commandName,
                    commandParameters,
                    out var result))
            {
                response = null;
                return false;
            }
            else
            {
                response = result.ToObject<T>();
                return true;
            }
        }
    }

    abstract public class ArchicadAccessorComponent
        : Component, IButtonComponent
    {
        public ArchicadAccessorComponent(
            string name,
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string>() { "Refresh" };
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
            this.m_attributes = new ButtonAttributes(this);
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
            IGH_DataAccess DA)
        {
            if (!AutoRefresh && !ManualRefreshRequested)
            {
                AddRuntimeMessage(
                    GH_RuntimeMessageLevel.Remark,
                    "Outdated, waiting for manual refresh");
                return;
            }

            Solve(DA);
        }

        protected abstract void Solve(
            IGH_DataAccess DA);

        public List<string> CapsuleButtonTexts { get; set; }
    }

    abstract public class ArchicadExecutorComponent
        : Component, IButtonComponent
    {
        public ArchicadExecutorComponent(
            string name,
            string nickname,
            string description,
            string subCategory)
            : base(
                name,
                nickname,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string>() { "Execute" };
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
            this.m_attributes = new ButtonAttributes(this);
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
            IGH_DataAccess DA);

        public List<string> CapsuleButtonTexts { get; set; }
    }

    abstract public class ButtonComponent : Component, IButtonComponent
    {
        private int _additionalWidth;

        public ButtonComponent(
            string name,
            string nickname,
            string description,
            string subCategory,
            string buttonText,
            int additionalWidth = 0)
            : base(
                name,
                nickname,
                description,
                subCategory)
        {
            CapsuleButtonTexts = new List<string>() { buttonText };
            _additionalWidth = additionalWidth;
            CreateAttributes();
        }

        public override void CreateAttributes()
        {
            this.m_attributes = new ButtonAttributes(
                this,
                2,
                _additionalWidth);
        }

        public abstract void OnCapsuleButtonPressed(
            int buttonIndex);

        public List<string> CapsuleButtonTexts { get; set; }
    }
}