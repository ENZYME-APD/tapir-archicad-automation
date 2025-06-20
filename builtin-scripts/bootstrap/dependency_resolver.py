import os
import sys
import subprocess
import importlib
from importlib.metadata import version, PackageNotFoundError
import tomllib

from ..utilities.tk_utils import show_ok_cancel_dialog

class UserCancelledError(Exception):
    """Raised when a user cancels an operation."""
    pass

def get_script_name_from_path(path: str) -> str:
    return os.path.splitext(os.path.basename(path))[0]

def load_dependencies(toml_path="dependencies.toml") -> dict:
    with open(toml_path, "rb") as f:
        return tomllib.load(f).get("scripts", {})

def get_installed_version(pkg_name: str) -> str | None:
    try:
        return version(pkg_name)
    except PackageNotFoundError:
        return None

def install_package(pkg_name: str, pkg_info: dict) -> None:
    pkg_version = pkg_info["version"]
    if not show_ok_cancel_dialog(
            message=f"Script dependency {pkg_name} - {pkg_version} is missing. \n"
                    f"Would you like to install it?",
            title="Missing dependency"):
        raise UserCancelledError(f"User cancelled the installation of {pkg_name}.")

    try:
        if url:= pkg_info.get("url"):
            subprocess.check_call([sys.executable, "-m", "pip", "install", url])
        else:
            subprocess.check_call([sys.executable, "-m", "pip", "install", f"{pkg_name}=={pkg_version}"])
    except  subprocess.CalledProcessError as e:
        print(f"ERROR: Failed to install {pkg_name}. pip exited with code {e.returncode}. \n {e.stderr}")


def ensure_dependencies(script_path: str) -> None:
    script_name = get_script_name_from_path(script_path)
    dependencies = load_dependencies().get(script_name, {})
    for pkg_name, pkg_info in dependencies.items():
        installed_version = get_installed_version(pkg_name)
        if compare_versions(installed_version, pkg_info["version"]) == - 1:
            install_package(pkg_name, pkg_info)
    importlib.invalidate_caches()

def compare_versions(installed: str, required: str) -> int:
    """
    Compares two simple, numeric version strings from left to right.

    Handles versions like "1.0", "2.11.5", "0.3".
    Does NOT handle pre-release tags like "1.0-alpha", "1.0rc1".

    Returns:
        -1 if installed < required
         0 if installed == required
         1 if installed > required
    """
    # Split versions into lists of numbers. Handle potential invalid characters.
    try:
        v1_parts = [int(p) for p in installed.split('.')]
        v2_parts = [int(p) for p in required.split('.')]
    except ValueError:
        raise ValueError(
            f"Invalid version string. Versions must be dot-separated numbers. "
            f"Got: '{installed}' and '{required}'"
        )

    # Compare parts from left to right. zip stops when the shorter list is exhausted.
    for p1, p2 in zip(v1_parts, v2_parts):
        if p1 > p2:
            return 1
        if p1 < p2:
            return -1

    # If all common parts are equal, the longer version is newer
    if len(v1_parts) > len(v2_parts):
        return 1
    if len(v1_parts) < len(v2_parts):
        return -1

    # If all parts are equal and lengths are the same, the versions are identical
    return 0
