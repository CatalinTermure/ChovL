#include "ast.h"

#include <llvm/IR/Verifier.h>

namespace chovl {

using llvm::BasicBlock;
using llvm::ConstantFP;
using llvm::ConstantInt;
using llvm::Function;
using llvm::FunctionType;

namespace {
llvm::Value* CastValue(Context& context, llvm::Value* src, llvm::Type* src_type,
                       llvm::Type* dst_type) {
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

AST::AST(ASTAggregateNode* root) { root_ = root; }

void AST::codegen() {
  std::vector<llvm::Value*> vals = root_->codegen(llvm_context);
  std::string type_str;
  llvm::raw_string_ostream rso(type_str);
  for (auto val : vals) {
    val->print(rso);
    rso << "\n";
  }
  std::cout << rso.str();
}

llvm::Value* I32Node::codegen(Context& context) {
  return context.llvm_builder->getInt32(value_);
}

llvm::Value* I64Node::codegen(Context& context) {
  return context.llvm_builder->getInt64(value_);
}

llvm::Value* F32Node::codegen(Context& context) {
  return ConstantFP::get(llvm::Type::getFloatTy(context.llvm_context), value_);
}

llvm::Value* F64Node::codegen(Context& context) {
  return ConstantFP::get(llvm::Type::getDoubleTy(context.llvm_context), value_);
}

llvm::Value* BinaryExprNode::codegen(Context& context) {
  llvm::Value* lhs = lhs_->codegen(context);
  llvm::Value* rhs = rhs_->codegen(context);

  switch (op_) {
    case Operator::kAdd:
      if (lhs->getType()->isFloatingPointTy()) {
        return context.llvm_builder->CreateFAdd(lhs, rhs, "addtmp");
      }
      return context.llvm_builder->CreateAdd(lhs, rhs, "addtmp");
    case Operator::kSub:
      if (lhs->getType()->isFloatingPointTy()) {
        return context.llvm_builder->CreateFSub(lhs, rhs, "addtmp");
      }
      return context.llvm_builder->CreateSub(lhs, rhs, "addtmp");
    default:
      return nullptr;
  }

  return nullptr;
}

TypeNode::TypeNode(Type type) : type_(type) {}

ParameterNode::ParameterNode(TypeNode* type, const char* name)
    : type_(type), name_(name) {}

std::vector<llvm::Value*> ASTListNode::codegen(Context& context) {
  std::vector<llvm::Value*> vals;
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

llvm::Value* FunctionDeclNode::codegen(Context& context) {
  std::vector<llvm::Type*> param_types;
  for (auto& param : params_->nodes()) {
    param_types.push_back(param->llvm_type(context));
  }

  FunctionType* func_type =
      FunctionType::get(return_type_->llvm_type(context), param_types, false);
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

llvm::Value* FunctionDefNode::codegen(Context& context) {
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

llvm::Value* CastOpNode::codegen(Context& context) {
  llvm::Type* dst_type = type_->llvm_type(context);
  llvm::Value* src = value_->codegen(context);
  llvm::Type* src_type = src->getType();

  return CastValue(context, src, src_type, dst_type);
}

FunctionCallNode::FunctionCallNode(const char* identifier,
                                   ASTAggregateNode* params)
    : identifier_(identifier), params_(params) {}

llvm::Value* FunctionCallNode::codegen(Context& context) {
  Function* func = context.llvm_module->getFunction(identifier_);
  if (!func) {
    llvm::errs() << "Function not found: " << identifier_ << "\n";
    return nullptr;
  }

  std::vector<llvm::Value*> args = params_->codegen(context);
  return context.llvm_builder->CreateCall(func, args);
}

BlockNode::BlockNode(ASTAggregateNode* body, bool is_void)
    : body_(body), is_void_(is_void) {}

llvm::Value* BlockNode::codegen(Context& context) {
  std::vector<llvm::Value*> vals = body_->codegen(context);

  if (is_void_ || vals.empty()) {
    return nullptr;
  }
  return vals.back();
}

llvm::Value* CharNode::codegen(Context& context) {
  return context.llvm_builder->getInt8(value_);
}

}  // namespace chovl
