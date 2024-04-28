#include "operators.h"

namespace chovl {

llvm::Value* CreateBinaryOperation(llvm::IRBuilder<>* builder, Operator op,
                                   llvm::Value* lhs, llvm::Value* rhs) {
  if (lhs->getType() != rhs->getType()) {
    std::string error_str = "BinaryExprNode: lhs and rhs types do not match: ";
    llvm::raw_string_ostream rso(error_str);
    lhs->getType()->print(rso);
    rso << " vs ";
    rhs->getType()->print(rso);
    throw std::runtime_error(rso.str());
  }

  switch (op) {
    case Operator::kAdd:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFAdd(lhs, rhs, "addtmp");
      }
      return builder->CreateAdd(lhs, rhs, "addtmp");
    case Operator::kSub:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFSub(lhs, rhs, "addtmp");
      }
      return builder->CreateSub(lhs, rhs, "addtmp");
    case Operator::kDiv:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFDiv(lhs, rhs, "divtmp");
      }
      return builder->CreateSDiv(lhs, rhs, "divtmp");
    case Operator::kMul:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFMul(lhs, rhs, "multmp");
      }
      return builder->CreateMul(lhs, rhs, "multmp");
    case Operator::kMod:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFRem(lhs, rhs, "modtmp");
      }
      return builder->CreateSRem(lhs, rhs, "modtmp");
    case Operator::kEq:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpUEQ(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpEQ(lhs, rhs, "cmptmp");
    case Operator::kNotEq:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpUNE(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpNE(lhs, rhs, "cmptmp");
    case Operator::kLessThan:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpULT(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpSLT(lhs, rhs, "cmptmp");
    case Operator::kGreaterThan:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpUGT(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpSGT(lhs, rhs, "cmptmp");
    case Operator::kLessEq:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpULE(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpSLE(lhs, rhs, "cmptmp");
    case Operator::kGreaterEq:
      if (lhs->getType()->isFloatingPointTy()) {
        return builder->CreateFCmpUGE(lhs, rhs, "cmptmp");
      }
      return builder->CreateICmpSGE(lhs, rhs, "cmptmp");
    case Operator::kAnd:
      return builder->CreateAnd(lhs, rhs, "andtmp");
    case Operator::kOr:
      return builder->CreateOr(lhs, rhs, "ortmp");
    default:
      return nullptr;
  }

  return nullptr;
}
}  // namespace chovl
