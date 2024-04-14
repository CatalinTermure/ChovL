#ifndef CHOVL_AST_H_
#define CHOVL_AST_H_

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <iostream>
#include <memory>
#include <unordered_map>

namespace chovl {

struct Context;

class ASTNode {
 public:
  virtual llvm::Value *codegen(chovl::Context &context) = 0;
  virtual ~ASTNode() = default;
};

class ASTAggregateNode {
 public:
  virtual std::vector<llvm::Value *> codegen(chovl::Context &context) = 0;
  virtual void push_back(ASTNode *node) = 0;
  virtual ~ASTAggregateNode() = default;
};

enum class Operator : uint8_t {
  kAdd,
  kSub,
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

enum class Primitive { kNone, kI32, kF32 };

class TypeNode {
 public:
  explicit TypeNode(Primitive primitive);

  llvm::Type *type(Context &context);

 private:
  Primitive primitive_;
};

class ParameterNode {
 public:
  ParameterNode(TypeNode *type, const char *name);

  llvm::Type *type(Context &context) { return type_->type(context); }
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

class ASTRootNode : public ASTAggregateNode {
 public:
  ASTRootNode() = default;

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

class CastOpNode : public ASTNode {
 public:
  CastOpNode(TypeNode *type, ASTNode *value);

  llvm::Value *codegen(Context &context) override;

 private:
  std::unique_ptr<TypeNode> type_;
  std::unique_ptr<ASTNode> value_;
};

struct Context {
  Context();
  ~Context() = default;

  llvm::LLVMContext llvm_context;
  std::unique_ptr<llvm::IRBuilder<>> llvm_builder;
  std::unique_ptr<llvm::Module> llvm_module;
  std::unordered_map<std::string, llvm::Value *> value_map;
  std::unordered_map<std::string, FunctionDeclNode *> function_map;
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

#endif  // CHOVL_AST_H_