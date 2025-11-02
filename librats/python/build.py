#!/usr/bin/env python3
"""
Build script for librats Python bindings.

This script helps with building and testing the Python bindings.
"""

import os
import sys
import subprocess
import argparse
import shutil
from pathlib import Path


def run_command(cmd, cwd=None, check=True):
    """Run a shell command."""
    print(f"Running: {cmd}")
    if isinstance(cmd, str):
        cmd = cmd.split()
    
    result = subprocess.run(cmd, cwd=cwd, capture_output=False)
    if check and result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}")
        sys.exit(1)
    return result


def build_native_library():
    """Build the native librats library."""
    project_root = Path(__file__).parent.parent
    build_dir = project_root / "build"
    
    print("Building native librats library...")
    
    # Create build directory
    if not build_dir.exists():
        build_dir.mkdir()
    
    # Run CMake
    run_command(["cmake", "-DRATS_SHARED_LIBRARY=ON", ".."], cwd=build_dir)
    
    # Build
    if os.name == 'nt':  # Windows
        run_command(["cmake", "--build", ".", "--config", "Release"], cwd=build_dir)
    else:  # Unix-like
        run_command(["make", "-j4"], cwd=build_dir)
    
    print("✓ Native library built successfully")


def install_python_package(development=True):
    """Install the Python package."""
    print("Installing Python package...")
    
    if development:
        run_command([sys.executable, "-m", "pip", "install", "-e", "."])
    else:
        run_command([sys.executable, "-m", "pip", "install", "."])
    
    print("✓ Python package installed successfully")


def run_tests():
    """Run the test suite."""
    print("Running tests...")
    
    # Install test dependencies
    run_command([sys.executable, "-m", "pip", "install", "-e", ".[dev]"])
    
    # Run tests
    run_command([sys.executable, "-m", "pytest", "librats_py/tests/", "-v"])
    
    print("✓ Tests completed")


def run_examples():
    """Test the examples."""
    print("Testing examples...")
    
    # Just import them to check for syntax errors
    examples = [
        "librats_py.examples.basic_client",
        "librats_py.examples.file_transfer", 
        "librats_py.examples.gossipsub_chat"
    ]
    
    for example in examples:
        try:
            run_command([sys.executable, "-c", f"import {example}"])
            print(f"✓ {example} imports successfully")
        except subprocess.CalledProcessError:
            print(f"✗ {example} failed to import")


def clean():
    """Clean build artifacts."""
    print("Cleaning build artifacts...")
    
    # Remove common build/cache directories
    to_remove = [
        "build",
        "dist", 
        "*.egg-info",
        "__pycache__",
        ".pytest_cache",
        ".coverage",
        "htmlcov"
    ]
    
    for pattern in to_remove:
        for path in Path(".").glob(f"**/{pattern}"):
            if path.is_dir():
                shutil.rmtree(path)
                print(f"Removed directory: {path}")
            else:
                path.unlink()
                print(f"Removed file: {path}")
    
    print("✓ Cleanup completed")


def package():
    """Create distribution packages."""
    print("Creating distribution packages...")
    
    # Install build dependencies
    run_command([sys.executable, "-m", "pip", "install", "build"])
    
    # Build packages
    run_command([sys.executable, "-m", "build"])
    
    print("✓ Distribution packages created in dist/")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description="Build script for librats Python bindings")
    parser.add_argument("--build-native", action="store_true", 
                       help="Build the native librats library")
    parser.add_argument("--install", action="store_true",
                       help="Install the Python package")
    parser.add_argument("--install-release", action="store_true",
                       help="Install the Python package (non-development)")
    parser.add_argument("--test", action="store_true",
                       help="Run the test suite")
    parser.add_argument("--examples", action="store_true", 
                       help="Test examples")
    parser.add_argument("--clean", action="store_true",
                       help="Clean build artifacts")
    parser.add_argument("--package", action="store_true",
                       help="Create distribution packages")
    parser.add_argument("--all", action="store_true",
                       help="Run all build steps")
    
    args = parser.parse_args()
    
    # Change to script directory
    os.chdir(Path(__file__).parent)
    
    try:
        if args.all:
            build_native_library()
            install_python_package(development=True)
            run_tests()
            run_examples()
        else:
            if args.build_native:
                build_native_library()
            
            if args.install:
                install_python_package(development=True)
            
            if args.install_release:
                install_python_package(development=False)
                
            if args.test:
                run_tests()
            
            if args.examples:
                run_examples()
            
            if args.clean:
                clean()
            
            if args.package:
                package()
    
    except KeyboardInterrupt:
        print("\n❌ Build interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Build failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
