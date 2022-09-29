#!/usr/bin/env python27
# -*- coding: utf-8 -*-

# - - - - - - - - LOCAL IMPORTS
from tapir_py import core, parts, utility,tools

# - - - - - - - - GLOBAL METHODS
def is_alive():
    """Checks if a connection can be established with ArchiCAD.
    
    Returns:
        bool: True if successful, False otherwise.
    
    """
    return core.Command.create().IsAlive()