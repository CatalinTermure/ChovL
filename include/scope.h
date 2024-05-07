#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "context.h"

namespace chovl {

enum class AggregateType { kSingular, kArray };
enum class PrimitiveType { kNone, kI32, kF32, kChar };

struct Type {
  explicit Type(PrimitiveType kind)
      : kind_(kind), aggregate_kind_(AggregateType::kSingular) {}
  Type(PrimitiveType kind, size_t size)
      : kind_(kind), aggregate_kind_(AggregateType::kArray), size_(size) {}

  explicit Type(llvm::Type *type);

  Type(const Type &) = default;
  Type &operator=(const Type &) = default;

  Type(Type &&) = default;
  Type &operator=(Type &&) = default;

  llvm::Type *llvm_type(Context &context) const;
  PrimitiveType kind() const { return kind_; }

 private:
  PrimitiveType kind_;
  AggregateType aggregate_kind_;
  size_t size_;
};

class SymbolicValue {
 public:
  SymbolicValue(llvm::Value *value, llvm::AllocaInst *alloca, Type type)
      : value_(value), alloca_(alloca), type_(type) {}

  SymbolicValue(const SymbolicValue &) = delete;
  SymbolicValue &operator=(const SymbolicValue &) = delete;

  SymbolicValue(SymbolicValue &&) noexcept;
  SymbolicValue &operator=(SymbolicValue &&) noexcept;

  llvm::Value *llvm_value() const { return value_; }
  llvm::AllocaInst *llvm_alloca() const { return alloca_; }
  llvm::Type *llvm_type(Context &context) const {
    return type_.llvm_type(context);
  }

 private:
  llvm::Value *value_;
  llvm::AllocaInst *alloca_;
  Type type_;
};

class SymbolTable {
 public:
  SymbolTable() = default;
  void AddSymbol(const std::string &name, SymbolicValue value);
  SymbolicValue &GetSymbol(const std::string &name);
  const SymbolicValue &GetSymbol(const std::string &name) const;
  void AddScope();
  void RemoveScope();

 private:
  std::vector<std::unordered_map<std::string, SymbolicValue>> symbols_;
};
}  // namespace chovl
