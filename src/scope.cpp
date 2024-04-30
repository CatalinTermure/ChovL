#include "scope.h"

namespace chovl {
Type::Type(llvm::Type* type) {
  aggregate_kind_ = AggregateType::kSingular;
  if (type->isIntegerTy(32)) {
    kind_ = PrimitiveType::kI32;
  } else if (type->isFloatTy()) {
    kind_ = PrimitiveType::kF32;
  } else if (type->isIntegerTy(8)) {
    kind_ = PrimitiveType::kChar;
  } else if (type->isVoidTy()) {
    kind_ = PrimitiveType::kNone;
  } else if (type->isArrayTy()) {
    auto array_type = llvm::cast<llvm::ArrayType>(type);
    size_ = array_type->getNumElements();
    if (array_type->getElementType()->isIntegerTy(32)) {
      kind_ = PrimitiveType::kI32;
    } else if (array_type->getElementType()->isFloatTy()) {
      kind_ = PrimitiveType::kF32;
    } else if (array_type->getElementType()->isIntegerTy(8)) {
      kind_ = PrimitiveType::kChar;
    }
    aggregate_kind_ = AggregateType::kArray;
  }
}

llvm::Type* Type::llvm_type(Context& context) const {
  switch (aggregate_kind_) {
    case AggregateType::kSingular:
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
      break;
    case AggregateType::kArray:
      switch (kind_) {
        case PrimitiveType::kI32:
          return llvm::ArrayType::get(llvm::Type::getInt32Ty(context.llvm_context), size_);
        case PrimitiveType::kF32:
          return llvm::ArrayType::get(llvm::Type::getFloatTy(context.llvm_context), size_);
        case PrimitiveType::kChar:
          return llvm::ArrayType::get(llvm::Type::getInt8Ty(context.llvm_context), size_);
        case PrimitiveType::kNone:
          return nullptr;
      }
      break;
  }

  return nullptr;
}

SymbolicValue::SymbolicValue(SymbolicValue&& other) noexcept
    : value_(other.value_), type_(other.type_), alloca_(other.alloca_) {
  other.value_ = nullptr;
}

SymbolicValue& SymbolicValue::operator=(SymbolicValue&& other) noexcept {
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
  for (auto it = symbols_.rbegin(); it != symbols_.rend(); ++it) {
    auto symbol = it->find(name);
    if (symbol != it->end()) {
      return symbol->second;
    }
  }
  throw std::runtime_error("Symbol not found");
}

const SymbolicValue& SymbolTable::GetSymbol(const std::string& name) const {
  for (auto it = symbols_.rbegin(); it != symbols_.rend(); ++it) {
    auto symbol = it->find(name);
    if (symbol != it->end()) {
      return symbol->second;
    }
  }
  throw std::runtime_error("Symbol not found");
}

void SymbolTable::AddScope() { symbols_.emplace_back(); }

void SymbolTable::RemoveScope() { symbols_.pop_back(); }

}  // namespace chovl
