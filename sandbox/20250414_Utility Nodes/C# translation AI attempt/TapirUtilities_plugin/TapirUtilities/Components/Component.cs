using Grasshopper.GUI;
using Grasshopper.GUI.Canvas;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Attributes;
using System;
using System.Drawing;
using System.Windows.Forms;

namespace Tapir.Components
{
    public interface IButtonComponent
    {
        void OnCapsuleButtonPressed();
        string CapsuleButtonText { get; set; }
    }

    public class ButtonAttributes : GH_ComponentAttributes
    {
        #region Fields
        private bool _isPressed = false;
        private RectangleF _buttonBounds = new RectangleF();
        #endregion Fields

        #region Constructor
        public ButtonAttributes(IGH_Component component) : base(component) { }
        #endregion Constructor

        #region Methods
        protected override void Layout()
        {
            base.Layout();

            Rectangle componentCapsule = GH_Convert.ToRectangle(this.Bounds);
            componentCapsule.Height += 30;

            Rectangle buttonCapsule = componentCapsule;
            buttonCapsule.Y = buttonCapsule.Bottom - 30;
            buttonCapsule.Height = 30;
            buttonCapsule.Inflate(-5, -5);

            this.Bounds = componentCapsule;
            this._buttonBounds = buttonCapsule;
        }

        protected override void Render(GH_Canvas canvas, Graphics graphics, GH_CanvasChannel channel)
        {
            base.Render(canvas, graphics, channel);

            if (channel == GH_CanvasChannel.Objects &&
                this.Owner is IButtonComponent buttonComponent)
            {
                GH_Capsule buttonCapsule = GH_Capsule.CreateTextCapsule(
                    box: this._buttonBounds,
                    textbox: this._buttonBounds,
                    palette: this._isPressed ? GH_Palette.Grey : GH_Palette.Black,
                    text: buttonComponent.CapsuleButtonText,
                    radius: 5,
                    highlight: 0);
                buttonCapsule.Render(graphics, this.Selected, this.Owner.Locked, false);
                buttonCapsule.Dispose();
            }
        }

        public override GH_ObjectResponse RespondToMouseDown(GH_Canvas sender, GH_CanvasMouseEvent e)
        {
            if (e.Button == MouseButtons.Left)
            {
                if (this._buttonBounds.Contains(e.CanvasLocation))
                {
                    this._isPressed = true;
                    sender.Refresh();

                    return GH_ObjectResponse.Handled;
                }
            }
            return base.RespondToMouseDown(sender, e);
        }

        public override GH_ObjectResponse RespondToMouseUp(GH_Canvas sender, GH_CanvasMouseEvent e)
        {
            if (e.Button == System.Windows.Forms.MouseButtons.Left)
            {
                if (this._buttonBounds.Contains(e.CanvasLocation) && this._isPressed == true)
                {
                    if (this.Owner is IButtonComponent buttonComponent)
                    {
                        buttonComponent.OnCapsuleButtonPressed();
                        this._isPressed = false;
                        sender.Refresh();
                    }
                    return GH_ObjectResponse.Release;
                }
            }
            return base.RespondToMouseUp(sender, e);
        }
        #endregion Methods
    }

    abstract public class Component : GH_Component
    {
        public Component(string name, string nickname, string description, string subCategory) :
            base(name, nickname, description, "Tapir", subCategory)
        {
        }
    }

    abstract public class ButtonComponent : Component, IButtonComponent
    {
        public ButtonComponent(string name, string nickname, string description, string subCategory) :
            base(name, nickname, description, subCategory)
        {
            CapsuleButtonText = "Button";
        }

        public override void CreateAttributes()
        {
            this.m_attributes = new ButtonAttributes(this);
        }

        public virtual void OnCapsuleButtonPressed()
        {
            // To be implemented by derived classes
        }

        public string CapsuleButtonText { get; set; }
    }
}
