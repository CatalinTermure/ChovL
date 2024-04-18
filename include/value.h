#pragma once

#include <llvm/IR/Type.h>

#include "context.h"

namespace chovl {

enum class PrimitiveType { kNone, kI32, kF32, kChar };

struct Type {
  // Intentionally not explicit to allow implicit conversion from PrimitiveType.
  Type(PrimitiveType kind) : kind_(kind) {}

  llvm::Type *llvm_type(Context &context) const;
  PrimitiveType kind() const { return kind_; }

 private:
  PrimitiveType kind_;
};
}  // namespace chovl
