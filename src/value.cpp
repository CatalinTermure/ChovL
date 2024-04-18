#include "value.h"

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

}  // namespace chovl
