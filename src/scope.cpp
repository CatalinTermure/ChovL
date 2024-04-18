#include "scope.h"

namespace chovl {
llvm::Type* Type::llvm_type(Context& context) const {
  switch (kind_) {
    case PrimitiveType::kI32:
      return llvm::Type::getInt32Ty(context.llvm_context);
    case PrimitiveType::kF32:
      return llvm::Type::getFloatTy(context.llvm_context);
    case PrimitiveType::kChar:
      return llvm::Type::getInt8Ty(context.llvm_context);
    case PrimitiveType::kNone:
      return llvm::Type::getVoidTy(context.llvm_context);
  }
  return nullptr;
}

SymbolicValue::SymbolicValue(SymbolicValue&& other)
    : value_(other.value_), type_(other.type_), alloca_(other.alloca_) {
  other.value_ = nullptr;
}

SymbolicValue& SymbolicValue::operator=(SymbolicValue&& other) {
  value_ = other.value_;
  type_ = other.type_;
  alloca_ = other.alloca_;
  other.value_ = nullptr;
  other.alloca_ = nullptr;
  return *this;
}

void SymbolTable::AddSymbol(const std::string& name, SymbolicValue value) {
  symbols_.back().emplace(name, std::move(value));
}

SymbolicValue& SymbolTable::GetSymbol(const std::string& name) {
  return symbols_.back().at(name);
}

const SymbolicValue& SymbolTable::GetSymbol(const std::string& name) const {
  return symbols_.back().at(name);
}

void SymbolTable::AddScope() { symbols_.emplace_back(); }

void SymbolTable::RemoveScope() { symbols_.pop_back(); }

}  // namespace chovl
