#pragma once

#include <cstdint>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace chovl {
enum class Operator : uint8_t {
  kAdd,
  kSub,
  kEq,
  kNotEq,
  kLessThan,
  kGreaterThan,
  kLessEq,
  kGreaterEq,
  kAnd,
  kOr
};

llvm::Value *CreateBinaryOperation(llvm::IRBuilder<> *builder, Operator op,
                                   llvm::Value *lhs, llvm::Value *rhs);

}  // namespace chovl
