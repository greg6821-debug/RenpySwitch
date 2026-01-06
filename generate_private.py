#!/usr/bin/env python3.9
"""
Generate private data archive for Ren'Py 8.3.7 on Switch
Compatible with Python 3.9
"""

import tarfile
import os
import sys

def make_tar(fn, source_dirs):
    """
    Creates a tar.gz archive from source directories.
    """
    print(f"Creating archive {fn} from directories: {source_dirs}")
    
    with tarfile.open(fn, "w:gz", format=tarfile.GNU_FORMAT) as tf:
        added = set()

        def add(fn_path, relfn):
            """Add file to archive with parent directories."""
            adds = []
            
            # Add parent directories first
            current_fn = fn_path
            current_relfn = relfn
            while current_relfn:
                adds.append((current_fn, current_relfn))
                current_fn = os.path.dirname(current_fn)
                current_relfn = os.path.dirname(current_relfn)
            
            adds.reverse()
            
            # Add directories and files
            for add_fn, add_relfn in adds:
                if add_relfn and add_relfn not in added:
                    added.add(add_relfn)
                    tf.add(add_fn, add_relfn, recursive=False)

        for sd in source_dirs:
            sd = os.path.abspath(sd)
            
            if not os.path.exists(sd):
                print(f"Warning: Source directory {sd} does not exist")
                continue
            
            print(f"Processing directory: {sd}")
            
            for root, dirs, files in os.walk(sd):
                # Add directories
                for dirname in dirs:
                    dir_path = os.path.join(root, dirname)
                    rel_path = os.path.relpath(dir_path, sd)
                    add(dir_path, rel_path)
                
                # Add files
                for filename in files:
                    file_path = os.path.join(root, filename)
                    rel_path = os.path.relpath(file_path, sd)
                    add(file_path, rel_path)
    
    print(f"Archive created: {fn}")
    print(f"Archive size: {os.path.getsize(fn) / 1024 / 1024:.2f} MB")

def main():
    """Main function."""
    print("Generating private data archive for Ren'Py 8.3.7 Switch")
    
    # Check if private directory exists
    if not os.path.exists("private"):
        print("Error: 'private' directory not found!")
        print("Please ensure you have built the project first.")
        sys.exit(1)
    
    # Create archive
    try:
        make_tar("private.mp3", ["private"])
        print("Successfully created private.mp3")
    except Exception as e:
        print(f"Error creating archive: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
