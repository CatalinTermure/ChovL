#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <unordered_map>

namespace chovl {
struct Context {
  Context();
  ~Context() = default;

  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::IRBuilder<>> llvm_builder;
  std::unique_ptr<llvm::Module> llvm_module;
};
}  // namespace chovl