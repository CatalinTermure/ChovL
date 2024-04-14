#include "ast.h"

#include <llvm/IR/Verifier.h>

namespace chovl {

using llvm::BasicBlock;
using llvm::ConstantFP;
using llvm::ConstantInt;
using llvm::Function;
using llvm::FunctionType;
using llvm::Type;
using llvm::Value;

namespace {
Value* CastValue(Context& context, Value* src, Type* src_type, Type* dst_type) {
  if (src_type == dst_type) {
    return src;
  }

  if (src_type->isIntegerTy() && dst_type->isIntegerTy()) {
    return context.llvm_builder->CreateIntCast(src, dst_type, true);
  }

  if (src_type->isFloatingPointTy() && dst_type->isFloatingPointTy()) {
    return context.llvm_builder->CreateFPCast(src, dst_type);
  }

  if (src_type->isIntegerTy() && dst_type->isFloatingPointTy()) {
    return context.llvm_builder->CreateSIToFP(src, dst_type);
  }

  if (src_type->isFloatingPointTy() && dst_type->isIntegerTy()) {
    return context.llvm_builder->CreateFPToSI(src, dst_type);
  }

  return nullptr;
}
}  // namespace

Context::Context() {
  llvm_builder = std::make_unique<llvm::IRBuilder<>>(llvm_context);
  llvm_module = std::make_unique<llvm::Module>("chovl", llvm_context);
}

AST::AST(ASTAggregateNode* root) { root_ = root; }

void AST::codegen() {
  std::vector<Value*> vals = root_->codegen(llvm_context);
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  for (auto val : vals) {
    val->print(rso);
    rso << "\n";
  }
  std::cout << rso.str();
}

Value* I32Node::codegen(Context& context) {
  return ConstantInt::get(Type::getInt32Ty(context.llvm_context), value_);
}

Value* I64Node::codegen(Context& context) {
  return ConstantInt::get(Type::getInt64Ty(context.llvm_context), value_);
}

Value* F32Node::codegen(Context& context) {
  return ConstantFP::get(Type::getFloatTy(context.llvm_context), value_);
}

Value* F64Node::codegen(Context& context) {
  return ConstantFP::get(Type::getDoubleTy(context.llvm_context), value_);
}

Value* BinaryExprNode::codegen(Context& context) {
  Value* lhs = lhs_->codegen(context);
  Value* rhs = rhs_->codegen(context);

  switch (op_) {
    case Operator::kAdd:
      if (lhs->getType()->isFloatingPointTy()) {
        return context.llvm_builder->CreateFAdd(lhs, rhs, "addtmp");
      }
      return context.llvm_builder->CreateAdd(lhs, rhs, "addtmp");
    default:
      return nullptr;
  }

  return nullptr;
}

TypeNode::TypeNode(Primitive primitive) { primitive_ = primitive; }

Type* TypeNode::type(Context& context) {
  switch (primitive_) {
    case Primitive::kI32:
      return Type::getInt32Ty(context.llvm_context);
    case Primitive::kF32:
      return Type::getFloatTy(context.llvm_context);
    case Primitive::kNone:
      return Type::getVoidTy(context.llvm_context);
  }

  return nullptr;
}

ParameterNode::ParameterNode(TypeNode* type, const char* name)
    : type_(type), name_(name) {}

std::vector<Value*> ASTRootNode::codegen(Context& context) {
  std::vector<Value*> vals;
  vals.reserve(nodes_.size());
  for (auto& node : nodes_) {
    vals.push_back(node->codegen(context));
  }
  return vals;
}

FunctionDeclNode::FunctionDeclNode(const char* identifier,
                                   ParameterListNode* params,
                                   TypeNode* return_type)
    : identifier_(identifier), params_(params), return_type_(return_type) {}

Value* FunctionDeclNode::codegen(Context& context) {
  std::vector<Type*> param_types;
  for (auto& param : params_->nodes()) {
    param_types.push_back(param->type(context));
  }

  FunctionType* func_type =
      FunctionType::get(return_type_->type(context), param_types, false);
  Function* func = Function::Create(func_type, Function::ExternalLinkage,
                                    identifier_, context.llvm_module.get());

  unsigned idx = 0;
  for (auto& arg : func->args()) {
    arg.setName(params_->nodes()[idx++]->name());
  }

  return func;
}

FunctionDefNode::FunctionDefNode(ASTNode* decl, ASTNode* body)
    : decl_(dynamic_cast<FunctionDeclNode*>(decl)), body_(body) {}

Value* FunctionDefNode::codegen(Context& context) {
  Function* func = static_cast<Function*>(decl_->codegen(context));
  if (!func) {
    return nullptr;
  }

  BasicBlock* block = BasicBlock::Create(context.llvm_context, "entry", func);
  context.llvm_builder->SetInsertPoint(block);

  context.llvm_builder->CreateRet(body_->codegen(context));

  llvm::verifyFunction(*func);

  return func;
}

CastOpNode::CastOpNode(TypeNode* type, ASTNode* value)
    : type_(type), value_(value) {}

Value* CastOpNode::codegen(Context& context) {
  Type* dst_type = type_->type(context);
  Value* src = value_->codegen(context);
  Type* src_type = src->getType();

  return CastValue(context, src, src_type, dst_type);
}

}  // namespace chovl
