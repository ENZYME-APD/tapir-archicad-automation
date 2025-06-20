from typing import Literal
import tkinter as tk
from tkinter import messagebox, simpledialog

def show_popup(message: str, title: str = "Script Message",
               message_type: Literal["error", "warning", "info"] = "error") -> None:
    """ Function to show a popup message """
    root = tk.Tk()
    root.withdraw()
    if message_type == "error":
        messagebox.showerror(title, message)
    elif message_type == "warning":
        messagebox.showwarning(title, message)
    else: # info or wrong type
        messagebox.showinfo(title, message)
    root.destroy()

def show_string_input_dialog(prompt: str, title: str = "Input Needed", default: str = "1")-> str:
    """ Function to show the string input dialog """
    root = tk.Tk()
    root.withdraw()
    response = simpledialog.askstring(title, prompt, initialvalue=default)
    root.destroy()
    return response

def show_ok_cancel_dialog(message: str, title: str = "Confirm") -> bool:
    """ Function to show a dialog with OK and Cancel buttons """
    root = tk.Tk()
    root.withdraw()
    result = messagebox.askokcancel(title, message)
    root.destroy()
    return result