#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/IR/DerivedTypes.h"

#include <sstream>

using namespace clang;

class CTypesVisitor : public RecursiveASTVisitor<CTypesVisitor> {
public:
  explicit CTypesVisitor(ASTContext *Context) : Context(Context), used_ndpointer(false) {}

  bool VisitFunctionDecl(FunctionDecl *Declaration) {

    // func_decls for initialization of args and outs in wrapper class __init__;
    // function wrappers for usage
    func_decls << "        self._lib." << Declaration->getNameAsString()
               << ".restype = "
               << typemap(Declaration->getReturnType())
               << "\n";
    func_wrappers << "    def " << Declaration->getNameAsString() << "(";

    func_decls << "        self._lib." << Declaration->getNameAsString()
               << ".argtypes = [";

    std::ostringstream paramlist;
    std::size_t ii = 0; // iteration number
    for (auto it = Declaration->param_begin(); it != Declaration->param_end(); ++it) {
      func_decls << typemap((*it)->getOriginalType());
      if ((*it)->getNameAsString() != "") {
        // Use the parameter's name if we have it
        func_wrappers << (*it)->getNameAsString();
        paramlist << (*it)->getNameAsString();
      }
      else {
        func_wrappers << "param" << ii;
        paramlist << "param" << ii;
      }
      if ((it+1) != Declaration->param_end()) {
        func_decls << ", ";
        func_wrappers << ", ";
        paramlist << ", ";
      }
      ++ii;
    }
    func_decls << "]" << "\n";
    func_wrappers << "):" << "\n";
    func_wrappers << "        ";
    if (typemap(Declaration->getReturnType()) != "None") {
      func_wrappers << "return ";
    }
    func_wrappers << "self._lib." << Declaration->getNameAsString()
                  << "(" << paramlist.str() << ")" << "\n";

    return true;
  }

  bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
    return true;
  }

  void HandleStruct(CXXRecordDecl *Declaration, const std::string & name) {
    forward_decls << "class " << name << "(ctypes.Structure):" << "\n"
                  << "    pass" << "\n";

    if (Declaration->field_empty()) {
      global_decls << name << "._fields_ = []\n\n";
    }
    else {
      global_decls << name << "._fields_ = [" << "\n";
      for (const auto & f : Declaration->fields()) {
        global_decls << "    ('" << f->getNameAsString() << "', " << typemap(f->getType()) << " " << "),\n";
      }
      global_decls << "]\n\n";
    }
  }

  bool VisitTypedefNameDecl(TypedefNameDecl *Declaration) {
    if (auto a = llvm::dyn_cast<clang::ElaboratedType>(Declaration->getUnderlyingType())) {
      if (a->isStructureType()) {
        if (auto d = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(a->getAsStructureType()->getDecl())) {
          if (d->getNameAsString() != "") {
            HandleStruct(d, d->getNameAsString());
          }
          else {
            HandleStruct(d, a->getNamedType().getAsString());
          }
        }
      }
    }
    return true;
  }

  void toString() {
    llvm::outs() << "# NOTE: This file was autogenerated using cythonator." << "\n"
                 << "#       Any changes will be overwritten upon regeneration." << "\n"
                 << "\n"
                 << "import ctypes\n";
    if (used_ndpointer) {
      llvm::outs() << "from numpy.ctypeslib import ndpointer\n";
    }

    llvm::outs() << "\n"
                 << forward_decls.str()
                 << "\n"
                 << global_decls.str();

    llvm::outs() << "\n"
                 << "class Wrapper:" << "\n"
                 << "    def __init__(self, lib_path):" << "\n"
                 << "        # Load the shared library" << "\n"
                 << "        self._lib = ctypes.cdll.LoadLibrary(lib_path)" << "\n"
                 << "\n"
                 << func_decls.str();
    llvm::outs() << "\n"
                 << func_wrappers.str();
  }

protected:
  const std::string typemap(const clang::QualType & t) {
    std::ostringstream rtype;
    bool is_ptr = t.getTypePtr()->isAnyPointerType();
    bool is_ptr_ptr = is_ptr && t.getTypePtr()->getPointeeType().getDesugaredType(*Context).getTypePtr()->isAnyPointerType();
    bool is_func_ptr = t.getTypePtr()->isFunctionPointerType();
    bool is_arr = t.getTypePtr()->isArrayType();
    bool is_const_arr = is_arr && llvm::dyn_cast_or_null<clang::ConstantArrayType>(t.getTypePtr());
    // bool is_varr = is_arr && llvm::dyn_cast_or_null<clang::VariableArrayType>(t.getTypePtr());
    // bool is_dep_size_arr = is_arr && llvm::dyn_cast_or_null<clang::DependentSizedArrayType>(t.getTypePtr());
    bool is_incomplete_arr = is_arr && llvm::dyn_cast_or_null<clang::IncompleteArrayType>(t.getTypePtr());

    // consider some special cases
    // TODO: recursively resolve pointer of pointer
    if (is_ptr) {
      const std::string & _rtype = t.getAsString();
      if (_rtype == "const char *") {
        is_ptr = false;
      }
      else if (_rtype == "char *") {
        is_ptr = false;
      }
      else if (_rtype == "void *") {
        is_ptr = false;
      }

      if (is_func_ptr) {
        rtype << "ctypes.CFUNCTYPE(";
      }
      else if (is_ptr) {
        rtype << "ctypes.POINTER(";
      }
    }

    if (is_incomplete_arr) {
      rtype << "ndpointer(";
      used_ndpointer = true;
    }

    const std::string & cmp_t = is_ptr
      ? (is_func_ptr
         ? llvm::dyn_cast_or_null<clang::FunctionType>(t.getTypePtr()->getPointeeType().getDesugaredType(*Context).getTypePtr())->getReturnType().getAsString()
         : (is_ptr_ptr
            ? typemap(t.getTypePtr()->getPointeeType().getUnqualifiedType())
            : t.getTypePtr()->getPointeeType().getUnqualifiedType().getAsString()))
      : (is_const_arr
         ? llvm::dyn_cast_or_null<clang::ConstantArrayType>(t.getTypePtr())->getElementType().getUnqualifiedType().getAsString()
         : (is_incomplete_arr
            ? llvm::dyn_cast_or_null<clang::IncompleteArrayType>(t.getTypePtr())->getElementType().getUnqualifiedType().getAsString()
            : t.getUnqualifiedType().getAsString()));

    if (cmp_t == "void") {
      rtype << "None";
    }
    else if (cmp_t == "int") {
      rtype << "ctypes.c_int";
    }
    else if (cmp_t == "double") {
      rtype << "ctypes.c_double";
    }
    else if (cmp_t == "const char *" || cmp_t == "char *") {
      rtype << "ctypes.c_char_p";
    }
    else if (cmp_t == "void *") {
      rtype << "ctypes.c_void_p";
    }
    else if (cmp_t == "size_t") {
      rtype << "ctypes.c_size_t";
    }
    else if (cmp_t == "long long") {
      rtype << "ctypes.c_longlong";
    }
    else if (cmp_t == "long double") {
      rtype << "ctypes.c_longdouble";
    }
    else if (cmp_t == "va_list") {
      // TODO: this may not work in all cases
      rtype << "ctypes.c_void_p";
    }
    else {
      // hacky workaround for typedef'd structs
      if (cmp_t.rfind("struct ", 0) == 0) {
        rtype << cmp_t.substr(7);
      }
      else {
        rtype << cmp_t;
      }
    }

    // TODO: handle arguments
    if (is_func_ptr) {
      if(auto func = llvm::dyn_cast_or_null<clang::FunctionProtoType>(t.getTypePtr()->getPointeeType().getDesugaredType(*Context).getTypePtr())) {
        for(const auto & p : func->getParamTypes()) {
          rtype << ", " << typemap(p);
        }
        // TODO: support varargs?
      }
    }

    if (auto a = llvm::dyn_cast_or_null<clang::ConstantArrayType>(t.getTypePtr())) {
      rtype << "*" << a->getSize().toString(/*radix=*/10, /*signed=*/false);
    }
    if (auto a = llvm::dyn_cast_or_null<clang::IncompleteArrayType>(t.getTypePtr())) {
      rtype << ", flags='C_CONTIGUOUS')";
    }
    if (is_ptr || is_func_ptr) {
      rtype << ")";
    }

    return rtype.str();
  }

private:
  ASTContext *Context;
  std::ostringstream func_decls;
  std::ostringstream func_wrappers;
  std::ostringstream forward_decls;
  std::ostringstream global_decls;
  bool used_ndpointer;
};

class TranspilationConsumer : public clang::ASTConsumer {
public:
  explicit TranspilationConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.toString();
  }
private:
  CTypesVisitor Visitor;
};

class FindDefines : public PPCallbacks {
public:
  void Defined(const Token & MacroNameTok, const MacroDefinition & MD, SourceRange Range) {
    llvm::outs() << "Found #define" << "\n";
  }
};

class TranspilationAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    Preprocessor &pp = Compiler.getPreprocessor();
    pp.addPPCallbacks(std::unique_ptr<FindDefines>());
    return std::unique_ptr<clang::ASTConsumer>(
        new TranspilationConsumer(&Compiler.getASTContext()));
  }
};

int main(int argc, char** argv) {
  if (argc > 1) {
    clang::tooling::runToolOnCode(std::make_unique<TranspilationAction>(), argv[1]);
  }
}
