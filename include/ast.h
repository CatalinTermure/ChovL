#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <iostream>
#include <memory>
#include <unordered_map>

#include "context.h"
#include "operators.h"
#include "scope.h"

namespace chovl {

// This class is never transmitted via a non-owning pointer, so all raw pointers
// to a Node are owning.
class ASTNode {
 public:
  virtual llvm::Value *codegen(Context &context) = 0;
  virtual ~ASTNode() = default;
};

class AssignableNode : public ASTNode {
 public:
  virtual llvm::Value *assign(Context &context, llvm::Value *value) = 0;
  virtual llvm::Value *llvm_alloca(Context &context) = 0;
  virtual Type type(Context &context) = 0;
  virtual ~AssignableNode() = default;
};

class MultiAssignableNode : public AssignableNode {
 public:
  virtual llvm::Value *assign(Context &context, llvm::Value *value) = 0;
  virtual llvm::Value *llvm_alloca(Context &context) = 0;
  virtual llvm::Value *multi_assign(Context &context,
                                    std::vector<llvm::Value *> value) = 0;
  virtual ~MultiAssignableNode() = default;
};

class ASTAggregateNode : public ASTNode {
 public:
  virtual llvm::Value *codegen(Context &context) override {
    throw std::runtime_error("Aggregate nodes cannot be codegen'd");
  }
  virtual std::vector<llvm::Value *> codegen_aggregate(Context &context) = 0;
  virtual void push_back(ASTNode *node) = 0;
  virtual ~ASTAggregateNode() = default;
};

class StringLiteralNode : public ASTNode {
 public:
  explicit StringLiteralNode(const char *value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  std::string value_;
};

class I32Node : public ASTNode {
 public:
  explicit I32Node(int32_t value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  int32_t value_;
};

class F32Node : public ASTNode {
 public:
  explicit F32Node(float value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  float value_;
};

class CharNode : public ASTNode {
 public:
  explicit CharNode(char value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  char value_;
};

class BinaryExprNode : public ASTNode {
 public:
  BinaryExprNode(Operator op, ASTNode *lhs, ASTNode *rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {}

  llvm::Value *codegen(Context &context) override;

 private:
  Operator op_;
  std::unique_ptr<ASTNode> lhs_;
  std::unique_ptr<ASTNode> rhs_;
};

class TypeNode {
 public:
  explicit TypeNode(Type type);

  Type get() { return type_; }
  llvm::Type *llvm_type(Context &context) { return type_.llvm_type(context); }

 private:
  Type type_;
};

class ParameterNode {
 public:
  ParameterNode(TypeNode *type, const char *name);

  llvm::Type *llvm_type(Context &context) { return type_->llvm_type(context); }
  std::string name() { return name_; }

 private:
  std::unique_ptr<TypeNode> type_;
  std::string name_;
};

class ParameterListNode {
 public:
  ParameterListNode() = default;

  std::vector<std::unique_ptr<ParameterNode>> &nodes() { return nodes_; }
  void push_back(ParameterNode *node) { nodes_.emplace_back(node); }

 private:
  std::vector<std::unique_ptr<ParameterNode>> nodes_;
};

class FunctionDeclNode : public ASTNode {
 public:
  FunctionDeclNode(const char *identifier, ParameterListNode *params,
                   TypeNode *return_type);

  llvm::Value *codegen(Context &context) override;

 private:
  std::string identifier_;
  std::unique_ptr<ParameterListNode> params_;
  std::unique_ptr<TypeNode> return_type_;
};

class ASTListNode : public ASTAggregateNode {
 public:
  ASTListNode() = default;

  void push_back(ASTNode *node) override { nodes_.emplace_back(node); }
  std::vector<llvm::Value *> codegen_aggregate(Context &context) override;

 private:
  std::vector<std::unique_ptr<ASTNode>> nodes_;
};

class FunctionDefNode : public ASTNode {
 public:
  FunctionDefNode(ASTNode *decl, ASTNode *body);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<FunctionDeclNode> decl_;
  std::unique_ptr<ASTNode> body_;
};

class FunctionCallNode : public ASTNode {
 public:
  FunctionCallNode(const char *identifier, ASTAggregateNode *params);

  llvm::Value *codegen(Context &context) override;

 private:
  std::string identifier_;
  std::unique_ptr<ASTAggregateNode> params_;
};

class CastOpNode : public ASTNode {
 public:
  CastOpNode(TypeNode *type, ASTNode *value);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<TypeNode> type_;
  std::unique_ptr<ASTNode> value_;
};

class BlockNode : public ASTNode {
 public:
  explicit BlockNode(ASTAggregateNode *body, bool is_void = false);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<ASTAggregateNode> body_;
  bool is_void_;
};

class VariableDeclarationNode : public ASTNode {
 public:
  VariableDeclarationNode(TypeNode *type, const char *name, ASTNode *value);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<TypeNode> type_;
  std::string name_;
  std::unique_ptr<ASTNode> value_;
};

class AssignmentNode : public ASTNode {
 public:
  AssignmentNode(AssignableNode *destination, ASTNode *value);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<AssignableNode> destination_;
  std::unique_ptr<ASTNode> value_;
};

class MultiAssignmentNode : public ASTNode {
 public:
  MultiAssignmentNode(AssignableNode *destination, ASTAggregateNode *values);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<MultiAssignableNode> destination_;
  std::unique_ptr<ASTAggregateNode> values_;
};

class VariableNode : public MultiAssignableNode {
 public:
  explicit VariableNode(const char *name);

  llvm::Value *codegen(Context &context) override;
  llvm::Value *assign(Context &context, llvm::Value *value) override;
  llvm::Value *llvm_alloca(Context &context) override;
  Type type(Context &context) override;
  llvm::Value *multi_assign(Context &context,
                            std::vector<llvm::Value *> values) override;

 private:
  std::string name_;
};

class VariableListNode : public MultiAssignableNode, public ASTAggregateNode {
 public:
  VariableListNode() = default;

  void push_back(ASTNode *node) override;
  llvm::Value *codegen(Context &context) override;
  std::vector<llvm::Value *> codegen_aggregate(Context &context) override;
  llvm::Value *assign(Context &context, llvm::Value *value) override;
  llvm::Value *llvm_alloca(Context &context) override;
  Type type(Context &context) override;
  llvm::Value *multi_assign(Context &context,
                            std::vector<llvm::Value *> values) override;

 private:
  std::vector<std::unique_ptr<AssignableNode>> nodes_;
};

class CondExprNode : public ASTNode {
 public:
  CondExprNode(ASTNode *cond, ASTNode *then, ASTNode *els);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<ASTNode> cond_;
  std::unique_ptr<ASTNode> then_;
  std::unique_ptr<ASTNode> else_;
};

class CondStatementNode : public ASTNode {
 public:
  CondStatementNode(ASTNode *cond, ASTNode *then, ASTNode *els);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<ASTNode> cond_;
  std::unique_ptr<ASTNode> then_;
  std::unique_ptr<ASTNode> else_;
};

class ArrayAccessNode : public AssignableNode {
 public:
  ArrayAccessNode(const char *name, ASTNode *index);

  llvm::Value *codegen(Context &context) override;
  llvm::Value *llvm_alloca(Context &context) override;
  Type type(Context &context) override;
  llvm::Value *assign(Context &context, llvm::Value *value) override;

 private:
  std::string name_;
  std::unique_ptr<ASTNode> index_;
};

class GetAddressNode : public ASTNode {
 public:
  explicit GetAddressNode(ASTNode *node);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<ASTNode> node_;
};

class DereferenceNode : public AssignableNode {
 public:
  explicit DereferenceNode(ASTNode *node);

  llvm::Value *codegen(Context &context) override;
  llvm::Value *llvm_alloca(Context &context) override;
  Type type(Context &context) override;
  llvm::Value *assign(Context &context, llvm::Value *value) override;

 private:
  std::unique_ptr<ASTNode> node_;
};

class AST {
 public:
  explicit AST(ASTAggregateNode *root);

  void codegen();

 private:
  Context llvm_context;
  std::unique_ptr<ASTAggregateNode> root_;
};

}  // namespace chovl