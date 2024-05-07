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

  std::string err_msg;
  llvm::raw_string_ostream rso(err_msg);
  rso << "Cannot cast from ";
  src_type->print(rso);
  rso << " to ";
  dst_type->print(rso);
  rso << "\n";

  throw std::runtime_error(rso.str());
}

llvm::Value* AssignValue(Context& context, llvm::Value* val, llvm::Value* ptr,
                         llvm::Type* type) {
  if (val->getType() != type) {
    val = CastValue(context, val, val->getType(), type);
  }
  return context.llvm_builder->CreateStore(val, ptr);
}

}  // namespace

AST::AST(ASTAggregateNode* root) : root_(root) {}

void AST::codegen() {
  std::vector<llvm::Value*> vals = root_->codegen_aggregate(llvm_context);
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

llvm::Value* F32Node::codegen(Context& context) {
  return ConstantFP::get(llvm::Type::getFloatTy(context.llvm_context), value_);
}

llvm::Value* BinaryExprNode::codegen(Context& context) {
  llvm::Value* lhs = lhs_->codegen(context);
  llvm::Value* rhs = rhs_->codegen(context);

  return CreateBinaryOperation(context.llvm_builder.get(), op_, lhs, rhs);
}

TypeNode::TypeNode(Type type) : type_(type) {}

ParameterNode::ParameterNode(TypeNode* type, const char* name)
    : type_(type), name_(name) {}

std::vector<llvm::Value*> ASTListNode::codegen_aggregate(Context& context) {
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
    std::cerr << "Function not found: " << identifier_ << "\n";
    return nullptr;
  }

  std::vector<llvm::Value*> args = params_->codegen_aggregate(context);
  return context.llvm_builder->CreateCall(func, args);
}

BlockNode::BlockNode(ASTAggregateNode* body, bool is_void)
    : body_(body), is_void_(is_void) {}

llvm::Value* BlockNode::codegen(Context& context) {
  context.symbol_table->AddScope();
  std::vector<llvm::Value*> vals = body_->codegen_aggregate(context);
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
  return context.llvm_builder->CreateLoad(sym.llvm_type(context),
                                          sym.llvm_alloca(), name_);
}

llvm::Value* VariableNode::assign(Context& context, llvm::Value* val) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  return AssignValue(context, val, sym.llvm_alloca(), sym.llvm_type(context));
}

llvm::Value* VariableNode::multi_assign(Context& context,
                                        std::vector<llvm::Value*> values) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  if (!sym.llvm_type(context)->isArrayTy()) {
    throw std::runtime_error("Cannot multi-assign to non-array type");
  }
  size_t size = sym.llvm_type(context)->getArrayNumElements();
  for (size_t i = 0; i < size; ++i) {
    llvm::Value* val = i < values.size() ? values[i] : values.back();
    auto element_type = sym.llvm_type(context)->getArrayElementType();
    llvm::Value* ptr = context.llvm_builder->CreateGEP(
        element_type, sym.llvm_alloca(), context.llvm_builder->getInt32(i));
    AssignValue(context, val, ptr, element_type);
  }

  return sym.llvm_value();
}

AssignmentNode::AssignmentNode(AssignableNode* destination, ASTNode* value)
    : destination_(destination), value_(value) {}

llvm::Value* AssignmentNode::codegen(Context& context) {
  llvm::Value* val = value_->codegen(context);
  return destination_->assign(context, val);
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

  if (else_) {
    curr_func->insert(curr_func->end(), else_block);
    context.llvm_builder->SetInsertPoint(else_block);
    else_->codegen(context);
    context.llvm_builder->CreateBr(merge_block);
  }

  curr_func->insert(curr_func->end(), merge_block);
  context.llvm_builder->SetInsertPoint(merge_block);

  return nullptr;
}

ArrayAccessNode::ArrayAccessNode(const char* name, ASTNode* index)
    : name_(name), index_(index) {}

llvm::Value* ArrayAccessNode::codegen(Context& context) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  auto element_type =
      llvm::cast<llvm::ArrayType>(sym.llvm_alloca()->getAllocatedType())
          ->getElementType();
  llvm::Value* idx = index_->codegen(context);
  llvm::Value* ptr =
      context.llvm_builder->CreateGEP(element_type, sym.llvm_alloca(), idx);
  return context.llvm_builder->CreateLoad(element_type, ptr);
}

llvm::Value* ArrayAccessNode::assign(Context& context, llvm::Value* val) {
  SymbolicValue& sym = context.symbol_table->GetSymbol(name_);
  auto element_type =
      llvm::cast<llvm::ArrayType>(sym.llvm_alloca()->getAllocatedType())
          ->getElementType();
  llvm::Value* idx = index_->codegen(context);
  llvm::Value* ptr =
      context.llvm_builder->CreateGEP(element_type, sym.llvm_alloca(), idx);
  return context.llvm_builder->CreateStore(val, ptr);
}

MultiAssignmentNode::MultiAssignmentNode(AssignableNode* destination,
                                         ASTAggregateNode* values)
    : destination_(dynamic_cast<MultiAssignableNode*>(destination)),
      values_(values) {}

llvm::Value* MultiAssignmentNode::codegen(Context& context) {
  return destination_->multi_assign(context,
                                    values_->codegen_aggregate(context));
}

llvm::Value* StringLiteralNode::codegen(Context& context) {
  return llvm::ConstantDataArray::getString(context.llvm_context, value_, true);
}

void VariableListNode::push_back(ASTNode* node) {
  AssignableNode* assignable = dynamic_cast<AssignableNode*>(node);
  if (assignable == nullptr) {
    throw std::runtime_error(
        "VariableListNode can only contain assignable nodes");
  }

  nodes_.emplace_back(assignable);
}

llvm::Value* VariableListNode::codegen(Context& context) {
  throw std::runtime_error("VariableListNode cannot be used as an expression");
}

std::vector<llvm::Value*> VariableListNode::codegen_aggregate(
    Context& context) {
  std::vector<llvm::Value*> vals;
  vals.reserve(nodes_.size());
  for (auto& node : nodes_) {
    vals.push_back(node->codegen(context));
  }
  return vals;
}

llvm::Value* VariableListNode::assign(Context& context, llvm::Value* value) {
  for (auto& node : nodes_) {
    node->assign(context, value);
  }

  return nullptr;
}

llvm::Value* VariableListNode::multi_assign(Context& context,
                                            std::vector<llvm::Value*> values) {
  if (values.size() != nodes_.size()) {
    throw std::runtime_error("Number of values does not match number of nodes");
  }

  for (size_t i = 0; i < nodes_.size(); ++i) {
    nodes_[i]->assign(context, values[i]);
  }

  return nullptr;
}

}  // namespace chovl
