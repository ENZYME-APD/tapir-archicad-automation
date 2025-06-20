import sys
import runpy

from dependency_resolver import ensure_dependencies

def main():
    if len(sys.argv) < 2:
        print("Usage: python bootstrap.py <script_to_run.py> [args...]")
        sys.exit(1)

    script_path = sys.argv[1]

    ensure_dependencies(script_path)

    # Adjust sys.argv to behave like running the target script directly
    sys.argv = [script_path] + sys.argv[2:]

    # Run the script in its own namespace
    runpy.run_path(script_path, run_name="__main__")

if __name__ == "__main__":
    main()