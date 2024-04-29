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

  return CreateBinaryOperation(context.llvm_builder.get(), op_, lhs, rhs);
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

  context.symbol_table->AddScope();
  // We need to create alloca for each argument to store them in the symbol
  // table. This gets optimized away by LLVM, so it's fine.
  for (auto& arg : func->args()) {
    llvm::AllocaInst* alloca = context.llvm_builder->CreateAlloca(
        arg.getType(), nullptr, arg.getName());
    context.llvm_builder->CreateStore(&arg, alloca);
    context.symbol_table->AddSymbol(std::string(arg.getName()),
                                    {&arg, alloca, Type(arg.getType())});
  }

  context.llvm_builder->CreateRet(body_->codegen(context));

  context.symbol_table->RemoveScope();

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
  context.symbol_table->AddScope();
  std::vector<llvm::Value*> vals = body_->codegen(context);
  context.symbol_table->RemoveScope();

  if (is_void_ || vals.empty()) {
    return nullptr;
  }
  return vals.back();
}

llvm::Value* CharNode::codegen(Context& context) {
  return context.llvm_builder->getInt8(value_);
}

VariableDeclarationNode::VariableDeclarationNode(TypeNode* type,
                                                 const char* name,
                                                 ASTNode* value)
    : type_(type), name_(name), value_(value) {}

llvm::Value* VariableDeclarationNode::codegen(Context& context) {
  llvm::Type* llvm_type = type_->llvm_type(context);

  llvm::Function* curr_func =
      context.llvm_builder->GetInsertBlock()->getParent();
  llvm::IRBuilder<> tmp_builder(&curr_func->getEntryBlock(),
                                curr_func->getEntryBlock().begin());

  llvm::AllocaInst* alloca =
      tmp_builder.CreateAlloca(llvm_type, nullptr, name_);

  llvm::Value* assigned_val = nullptr;
  if (value_ != nullptr) {
    llvm::Value* assigned_val = value_->codegen(context);
    context.llvm_builder->CreateStore(assigned_val, alloca);
  }
  context.symbol_table->AddSymbol(name_, {assigned_val, alloca, type_->get()});
  return nullptr;
}

VariableNode::VariableNode(const char* name) : name_(name) {}

llvm::Value* VariableNode::codegen(Context& context) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  return context.llvm_builder->CreateLoad(sym.llvm_alloca()->getAllocatedType(),
                                          sym.llvm_alloca(), name_);
}

VariableAssignmentNode::VariableAssignmentNode(const char* name, ASTNode* value)
    : name_(name), value_(value) {}

llvm::Value* VariableAssignmentNode::codegen(Context& context) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  llvm::Value* val = value_->codegen(context);
  return context.llvm_builder->CreateStore(val, sym.llvm_alloca());
}

CondExprNode::CondExprNode(ASTNode* cond, ASTNode* then, ASTNode* els)
    : cond_(cond), then_(then), else_(els) {}

llvm::Value* CondExprNode::codegen(Context& context) {
  Function* curr_func = context.llvm_builder->GetInsertBlock()->getParent();

  BasicBlock* then_block =
      BasicBlock::Create(context.llvm_context, "then", curr_func);
  BasicBlock* else_block = BasicBlock::Create(context.llvm_context, "else");
  BasicBlock* merge_block = BasicBlock::Create(context.llvm_context, "ifcont");

  llvm::Value* cond_val = cond_->codegen(context);
  if (else_) {
    context.llvm_builder->CreateCondBr(cond_val, then_block, else_block);
  } else {
    context.llvm_builder->CreateCondBr(cond_val, then_block, merge_block);
  }

  context.llvm_builder->SetInsertPoint(then_block);
  llvm::Value* then_val = then_->codegen(context);
  if (!then_val) {
    return nullptr;
  }

  context.llvm_builder->CreateBr(merge_block);
  then_block = context.llvm_builder->GetInsertBlock();

  llvm::Value* else_val = nullptr;
  if (else_) {
    curr_func->insert(curr_func->end(), else_block);
    context.llvm_builder->SetInsertPoint(else_block);
    else_val = else_->codegen(context);
    context.llvm_builder->CreateBr(merge_block);
    else_block = context.llvm_builder->GetInsertBlock();
  }

  curr_func->insert(curr_func->end(), merge_block);
  context.llvm_builder->SetInsertPoint(merge_block);

  llvm::PHINode* phi_node =
      context.llvm_builder->CreatePHI(then_val->getType(), 2, "iftmp");

  phi_node->addIncoming(then_val, then_block);
  if (else_) {
    phi_node->addIncoming(else_val, else_block);
  }

  return phi_node;
}

CondStatementNode::CondStatementNode(ASTNode* cond, ASTNode* then, ASTNode* els)
    : cond_(cond), then_(then), else_(els) {}

llvm::Value* CondStatementNode::codegen(Context& context) {
  Function* curr_func = context.llvm_builder->GetInsertBlock()->getParent();

  BasicBlock* then_block =
      BasicBlock::Create(context.llvm_context, "then", curr_func);
  BasicBlock* else_block = BasicBlock::Create(context.llvm_context, "else");
  BasicBlock* merge_block = BasicBlock::Create(context.llvm_context, "ifcont");

  llvm::Value* cond_val = cond_->codegen(context);
  if (else_) {
    context.llvm_builder->CreateCondBr(cond_val, then_block, else_block);
  } else {
    context.llvm_builder->CreateCondBr(cond_val, then_block, merge_block);
  }

  context.llvm_builder->SetInsertPoint(then_block);
  then_->codegen(context);
  context.llvm_builder->CreateBr(merge_block);
  then_block = context.llvm_builder->GetInsertBlock();

  if (else_) {
    curr_func->insert(curr_func->end(), else_block);
    context.llvm_builder->SetInsertPoint(else_block);
    else_->codegen(context);
    context.llvm_builder->CreateBr(merge_block);
    else_block = context.llvm_builder->GetInsertBlock();
  }

  curr_func->insert(curr_func->end(), merge_block);
  context.llvm_builder->SetInsertPoint(merge_block);

  return nullptr;
}

}  // namespace chovl
