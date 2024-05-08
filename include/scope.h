#pragma once

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "context.h"

namespace chovl {

enum class AggregateType : uint8_t { kSingular, kArray };
enum class IndirectionType : uint8_t { kNone, kPointer };
enum class PrimitiveType : uint8_t { kNone, kI32, kF32, kChar };

struct Type {
  Type(PrimitiveType kind, IndirectionType indirection)
      : kind_(kind),
        aggregate_kind_(AggregateType::kSingular),
        indirection_(indirection),
        size_(1) {}
  Type(PrimitiveType kind, size_t size, IndirectionType indirection)
      : kind_(kind),
        aggregate_kind_(AggregateType::kArray),
        size_(size),
        indirection_(indirection) {}

  explicit Type(llvm::Type *type);

  llvm::Type *llvm_type(Context &context) const;
  PrimitiveType kind() const { return kind_; }
  IndirectionType indirection() const { return indirection_; }

 private:
  PrimitiveType kind_;
  AggregateType aggregate_kind_;
  IndirectionType indirection_;
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
  Type type() const { return type_; }

 private:
  llvm::Value *value_;
  llvm::AllocaInst *alloca_;
  Type type_;
};

class SymbolTable {
 public:
  void AddSymbol(const std::string &name, SymbolicValue value);
  SymbolicValue &GetSymbol(const std::string &name);
  const SymbolicValue &GetSymbol(const std::string &name) const;
  void AddScope();
  void RemoveScope();

 private:
  std::vector<std::unordered_map<std::string, SymbolicValue>> symbols_;
};
}  // namespace chovl
