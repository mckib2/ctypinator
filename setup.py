'''Auto-build ctypinator.'''

from setuptools import find_packages
from distutils.core import Extension, setup

from clangTooling import (
    include_dir as clang_include_dir,
    library_dir as clang_library_dir,
)

libs = [
    'LLVMTableGenGlobalISel',
    'LLVMTableGen',
    'LLVMFrontendOpenMP',
    'LLVMOption',
    'LLVMTransformUtils',
    'LLVMAnalysis',
    'LLVMProfileData',
    'LLVMObject',
    'LLVMTextAPI',
    'LLVMBitReader',
    'LLVMCore',
    'LLVMRemarks',
    'LLVMBitstreamReader',
    'LLVMMCParser',
    'LLVMMC',
    'LLVMDebugInfoCodeView',
    'LLVMDebugInfoMSF',
    'LLVMBinaryFormat',
    'LLVMSupport',
    'LLVMDemangle',
]

clang_hdrs = clang_include_dir()

setup(
    name='ctypinator',
    version='0.0.1',
    author='Nicholas McKibben',
    author_email='nicholas.bgp@gmail.com',
    url='https://github.com/mckib2/ctypinator',
    license='MIT',
    description='Transpile C headers to Python ctypes wrapper',
    long_description=open('README.rst', encoding='utf-8').read(),
    packages=find_packages(),
    keywords='clang-tooling ctypes python',
    install_requires=open('requirements.txt', encoding='utf-8').read().split(),
    python_requires='>=3.5',

    ext_modules=[
        Extension(
            'ctypinator',
            sources=['src/transpiler.cpp'],
            include_dirs=[
                clang_hdrs / 'clang/include',
                clang_hdrs / 'build/tools/clang/include',
                clang_hdrs / 'llvm/include',
                clang_hdrs / 'build/include',
            ],
            library_dirs=[clang_library_dir()],
            libraries=libs,
            language='c++',
        )
    ],
)
