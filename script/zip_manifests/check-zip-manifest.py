#!/usr/bin/env python

import zipfile
import sys

def main(zip_path, manifest_in):
  with open(manifest_in, 'r') as manifest, \
      zipfile.ZipFile(zip_path, 'r', allowZip64=True) as z:
    files_in_zip = set(z.namelist())
    files_in_manifest = set([l.strip() for l in manifest.readlines()])
  added_files = files_in_zip - files_in_manifest
  removed_files = files_in_manifest - files_in_zip
  if added_files:
    print("Files added to bundle:")
    for f in sorted(list(added_files)):
      print('+' + f)
  if removed_files:
    print("Files removed from bundle:")
    for f in sorted(list(removed_files)):
      print('-' + f)
  if added_files or removed_files:
    return 1
  else:
    return 0

if __name__ == '__main__':
  sys.exit(main(*sys.argv[1:]))
