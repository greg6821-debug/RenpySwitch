#!/usr/bin/env python3
"""Generate private.tar.gz archive for Ren'Py 8.3.7 Switch builds."""

import tarfile
import os
import sys


def make_tar(fn: str, source_dirs: list) -> None:
    """
    Create a tar.gz archive from source directories.
    
    Args:
        fn: Output filename (should end with .tar.gz)
        source_dirs: List of directories to include in archive
    """
    
    # Ensure output filename has correct extension
    if not fn.endswith('.tar.gz'):
        fn += '.tar.gz'
    
    print(f"Creating archive: {fn}")
    print(f"Source directories: {source_dirs}")
    
    with tarfile.open(fn, "w:gz", format=tarfile.GNU_FORMAT) as tf:
        added = set()
        
        def add_entry(fn_path: str, rel_path: str) -> None:
            """Add a single entry to the archive."""
            # Generate all parent directories
            parts = []
            current_fn = fn_path
            current_rel = rel_path
            
            while current_rel:
                parts.append((current_fn, current_rel))
                current_fn = os.path.dirname(current_fn)
                current_rel = os.path.dirname(current_rel)
            
            parts.reverse()
            
            # Add directories from root to leaf
            for fn_to_add, rel_to_add in parts:
                if rel_to_add and rel_to_add not in added:
                    added.add(rel_to_add)
                    try:
                        tf.add(fn_to_add, rel_to_add, recursive=False)
                    except FileNotFoundError:
                        print(f"Warning: {fn_to_add} not found, skipping")
                    except Exception as e:
                        print(f"Warning: Could not add {fn_to_add}: {e}")
        
        # Process each source directory
        for source_dir in source_dirs:
            if not os.path.exists(source_dir):
                print(f"Error: Source directory does not exist: {source_dir}")
                sys.exit(1)
            
            source_dir = os.path.abspath(source_dir)
            print(f"Processing: {source_dir}")
            
            for root, dirs, files in os.walk(source_dir):
                # Add directories first
                for dirname in dirs:
                    full_path = os.path.join(root, dirname)
                    rel_path = os.path.relpath(full_path, source_dir)
                    add_entry(full_path, rel_path)
                
                # Add files
                for filename in files:
                    full_path = os.path.join(root, filename)
                    rel_path = os.path.relpath(full_path, source_dir)
                    add_entry(full_path, rel_path)
    
    print(f"Archive created successfully: {fn}")
    print(f"Total entries: {len(added)}")


if __name__ == "__main__":
    # Default behavior for Switch build
    if len(sys.argv) > 1:
        # Allow custom output filename and directories via command line
        output_file = sys.argv[1]
        source_dirs = sys.argv[2:] if len(sys.argv) > 2 else ["private"]
    else:
        # Default for Switch build process
        output_file = "private.mp3"  # Note: Actually creates .tar.gz
        source_dirs = ["private"]
    
    make_tar(output_file, source_dirs)
