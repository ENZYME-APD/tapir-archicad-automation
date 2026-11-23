"""A Python client for the Tapir Archicad Add-On: one method per Tapir command.

    from tapir_archicad import Client
    tapir = Client ()
    print (tapir.get_add_on_version ())
"""

from .client import Client, TapirError

# The package's version, read by pyproject.toml. The release workflow sets it to the Tapir release's
# version before it builds the package for PyPI; a checkout of the repository carries 0.0.0.dev0.
__version__ = '0.0.0.dev0'

__all__ = ['Client', 'TapirError', '__version__']
