#pragma once

#include <llvm/IR/ValueHandle.h>

#include <string>

namespace chovl {
class SymbolTable {
 public:
  SymbolTable();
  void AddSymbol(const std::string& name, llvm::Value* value,
                 llvm::Function* destructor);
  llvm::Value* GetSymbol(const std::string& name);
  void AddScope();
  void RemoveScope();

 private:
  std::vector<std::unordered_map<std::string, llvm::Value*>> symbols_;
};
}  // namespace chovl
