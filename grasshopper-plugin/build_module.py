#!/usr/bin/env ipy
# -*- coding: utf-8 -*-

"""
This script collects all python scripts (.py) from the project (including sub-folders) and compiles them into a dynamic link library (.dll) file. File will be placed inside the 'bin' folder. Place this file in the root directory of a project, prior to execution.

Note:
    This script uses the Common Language Runtime Library (CLR). Use IronPython for execution.
    Download IronPython 2.7.9 from https://github.com/IronLanguages/ironpython2/releases/tag/ipy-2.7.9

Usage:
    build_module.py

Author:
    Kaushik LS - 18.08.2022

Source:
    https://gist.github.com/thekaushikls/58a0727a86fb2e74121a782e123d163e

License:
    MIT License

    Copyright (c) 2022 Kaushik LS

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
"""

# - - - - - - - - IN-BUILT IMPORTS
import os
from traceback import format_exc
from distutils.dir_util import copy_tree
from shutil import rmtree, copy2

# - - - - - - - - CLASS LIBRARY
class Compiler:

    @staticmethod
    def collect_files(folder_path):
        """Collects relevant python script files.

        Args:
            folder_path (str): Root directory to collect_files.

        Returns:
            list(str): List of absolute file paths.

        """
        files = []
        # Ignore the current file, and files named '__init__.py'. Add other names if required.
        ignore_list = (os.path.basename(__file__), "__init__.py", "local", "bin")

        if os.path.isdir(folder_path):
            for file in os.listdir(folder_path):
                abs_path = os.path.join(folder_path, file)
                # ignore from ignore_list.
                if not file in ignore_list and file.endswith(".py"):
                    files.append(abs_path)
                # Recursiion for sub-folders
                elif os.path.isdir(abs_path) and file.lower() not in ignore_list:
                    files += Compiler.collect_files(abs_path)
        
        return files
    
    @staticmethod
    def Build(filename, source_folder = "", copy_target = "", export_folder = "bin"):
        """Collects and compiles project files into a dll.

        Args:
            filename (str): Name of final output (.dll) file.
            source_folder (str): (optional) Folder to collect files from.
            copy_target (str) : (optional) File path to where the output needs to be copied.
            export_folder (str): Subfolder for output file.

        Returns:
            (bool) True if successful, False otherwise.

        """
        folder_name = os.path.dirname(os.path.abspath(__file__))
        source_folder = os.path.join(folder_name, source_folder)
        target_folder = os.path.join(folder_name, export_folder)
        target_file = os.path.join(target_folder, filename)

        # Create export folder.
        if not os.path.exists(target_folder):
            os.makedirs(target_folder)
        
        # Collect necessary files from project folder.
        program_files = Compiler.collect_files(source_folder)
        print("\nCollected {} files".format(len(program_files)))

        try:
            from clr import CompileModules
            CompileModules(target_file, *program_files)
            print("BUILD SUCCESSFUL\nTarget: {}\n".format(target_file))
            
            # Copy output file to user specified location.
            if copy_target and os.path.exists(copy_target):
                copy2(target_file, copy_target)
                print("COPY SUCCESSFUL\n")
            
            return True

        except ImportError:
            print("\nBUILD FAILED\n\nPlease run the script using IronPython.\nRefer docstrings for more information.\n")

        except Exception as ex:
            print("\nBUILD FAILED\n")
            print(format_exc())
            print(str(ex))
            return False

# - - - - - - - - RUN SCRIPT
def Run(target):
    # Copy python-package pre-build.
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    package_source = os.path.join(root, 'python-package', 'src', 'tapir_py')
    package_target = os.path.join(root, 'grasshopper-plugin', 'src', 'tapir_py')
    if os.path.exists(package_source):
        copy_tree(package_source, package_target)
        # Build grasshopper-plugin.
        Compiler.Build(target, source_folder='src')
        # Clean up post-build
        rmtree(package_target)
    else:
        print("\nBUILD FAILED\n'tapir_py' package not found.\n")

if __name__ == "__main__":
    
    target='tapir_2909.ghpy'
    Run(target)
