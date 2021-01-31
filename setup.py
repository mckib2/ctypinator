'''Auto-build ctypinator.'''

from setuptools import find_packages
from distutils.core import Extension, setup

from clangTooling import (
    clang_includes,
    llvm_includes,
    library_dir,
    llvm_library_list,
)
from clangTooling.lib import clang_library_list

# create generated functions:
with open('src/no_python_keywords.hpp', 'w') as fp:
    import keyword
    keywords = set(f'"{name}"' for name in dir(__builtins__))
    keywords |= set(f'"{kw}"' for kw in keyword.kwlist)
    keywords = ', '.join(keywords)

    fp.write('\n'.join([
        '#ifndef NO_PYTHON_KEYWORDS_HPP',
        '#define NO_PYTHON_KEYWORDS_HPP',
        '',
        '#include <string>',
        '#include <set>',
        '',
        'std::string cleanPythonKeywords(const std::string& word) {',
        f'    static std::set<std::string> keywords = {{{keywords}}};',
        '    if (keywords.find(word) != keywords.end()) {',
        '        return word + "_";',
        '    }',
        '    return word;',
        '}',
        '',
        '#endif // NO_PYTHON_KEYWORDS_HPP',
    ]))


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

    # TODO: not portable to windows yet, but clangTooling currently not
    #       working on windows either...
    ext_modules=[
        Extension(
            'transpiler',
            sources=['src/transpiler.cpp'],
            include_dirs=clang_includes() + llvm_includes(),
            library_dirs=[str(library_dir())],
            libraries=clang_library_list() + llvm_library_list() + ['z', 'curses'],
            # runtime_library_dirs=[str(library_dir())],
            language='c++',
            extra_compile_args=['-fno-exceptions', '-fno-rtti'],
            extra_link_args=[],
            define_macros=[
                ('_GNU_SOURCE', None),
                ('__STDC_CONSTANT_MACROS', None),
                ('__STDC_FORMAT_MACROS', None),
                ('__STDC_LIMIT_MACROS', None),
                ('_GLIBCXX_USE_CXX11_ABI', '0'),
            ],
            depends=['src/no_python_keywords.hpp'],
        )
    ],
)
