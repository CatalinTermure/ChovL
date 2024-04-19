#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <iostream>
#include <memory>
#include <unordered_map>

#include "context.h"
#include "scope.h"

namespace chovl {

class ASTNode {
 public:
  virtual llvm::Value *codegen(Context &context) = 0;
  virtual ~ASTNode() = default;
};

class ASTAggregateNode {
 public:
  virtual std::vector<llvm::Value *> codegen(Context &context) = 0;
  virtual void push_back(ASTNode *node) = 0;
  virtual ~ASTAggregateNode() = default;
};

enum class Operator : uint8_t {
  kAdd,
  kSub,
  kLessThan,
  kGreaterThan,
  kLessEq,
  kGreaterEq,
};

class I32Node : public ASTNode {
 public:
  explicit I32Node(int32_t value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  int32_t value_;
};

class I64Node : public ASTNode {
 public:
  explicit I64Node(int64_t value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  int64_t value_;
};

class F32Node : public ASTNode {
 public:
  explicit F32Node(float value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  float value_;
};

class F64Node : public ASTNode {
 public:
  explicit F64Node(double value) : value_(value) {}

  llvm::Value *codegen(Context &context) override;

 private:
  double value_;
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
  const char *identifier_;
  std::unique_ptr<ParameterListNode> params_;
  std::unique_ptr<TypeNode> return_type_;
};

class ASTListNode : public ASTAggregateNode {
 public:
  ASTListNode() = default;

  void push_back(ASTNode *node) override { nodes_.emplace_back(node); }
  std::vector<llvm::Value *> codegen(Context &context) override;

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
  BlockNode(ASTAggregateNode *body, bool is_void = false);

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

class VariableAssignmentNode : public ASTNode {
 public:
  VariableAssignmentNode(const char *name, ASTNode *value);

  llvm::Value *codegen(Context &context) override;

 private:
  std::string name_;
  std::unique_ptr<ASTNode> value_;
};

class VariableNode : public ASTNode {
 public:
  explicit VariableNode(const char *name);

  llvm::Value *codegen(Context &context) override;

 private:
  std::string name_;
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

class AST {
 public:
  explicit AST(ASTAggregateNode *root);

  void codegen();

 private:
  Context llvm_context;
  ASTAggregateNode *root_;
};

}  // namespace chovl