"""A Python client for the Tapir Archicad Add-On: one method per Tapir command.

    from tapir import Client
    tapir = Client ()
    print (tapir.get_add_on_version ())
"""

from .client import Client, TapirError

__all__ = ['Client', 'TapirError']
