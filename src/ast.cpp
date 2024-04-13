#include "ast.h"

namespace chovl {

using llvm::ConstantFP;
using llvm::ConstantInt;
using llvm::Type;
using llvm::Value;
using llvm::FunctionType;
using llvm::Function;

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

  FunctionType* func_type = FunctionType::get(return_type_->type(context),
                                              param_types, false);
  Function* func = Function::Create(func_type, Function::ExternalLinkage,
                                     identifier_, context.llvm_module.get());

  unsigned idx = 0;
  for (auto& arg : func->args()) {
    arg.setName(params_->nodes()[idx++]->name());
  }

  return func;
}

}  // namespace chovl
