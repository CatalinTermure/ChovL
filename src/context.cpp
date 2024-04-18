#include "context.h"

#include "scope.h"

namespace chovl {
Context::Context() {
  llvm_builder = std::make_unique<llvm::IRBuilder<>>(llvm_context);
  llvm_module = std::make_unique<llvm::Module>("chovl", llvm_context);
  symbol_table = std::make_unique<SymbolTable>();
}
}  // namespace chovl
