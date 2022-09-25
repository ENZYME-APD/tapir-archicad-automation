#!/usr/bin/env python27
# -*- coding: utf-8 -*-

# - - - - - - - - LOCAL IMPORTS
from tapir_py import core, parts, utility

# - - - - - - - - GLOBAL METHODS
def is_alive():
    return core.Command.create().IsAlive()