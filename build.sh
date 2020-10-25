clang++-10 `llvm-config --cxxflags` \
           -c -fpic src/transpiler.cpp


clang++-10 -shared -o libctypinator.so transpiler.o \
           -lclangTooling -lclangASTMatchers -lclangFormat -lclangFrontend -lclangDriver \
           -lclangParse -lclangSerialization -lclangSema -lclangEdit \
           -lclangAnalysis -lclangToolingCore -lclangAST \
           -lclangRewrite -lclangLex -lclangBasic \
           `llvm-config --ldflags --libs --link-static` \
           -lz -lcurses -pthread
