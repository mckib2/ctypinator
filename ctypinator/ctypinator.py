'''Transpile C header files to Python ctypes wrappers.'''

import ctypes
import importlib.util
import argparse


def main(libpath: str, header: str, outfile: str):
    _lib = ctypes.cdll.LoadLibrary(importlib.util.find_spec('transpiler').origin)
    _lib.transpile.restype = None
    _lib.transpile.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
    with open(header, 'rb') as fp:
        code = fp.read()
        _lib.transpile(libpath.encode(), code, outfile.encode())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--libpath', type=str, help='Path to library file.', required=True)
    parser.add_argument('--header', type=str, help='C header file.', required=True)
    parser.add_argument('--output', type=str, help='Python wrapper file.', default='wrapper.py')
    args = parser.parse_args()
    main(libpath=args.libpath, header=args.header, outfile=args.output)
