#ifndef _WGTCC_CODE_GEN_H_
#define _WGTCC_CODE_GEN_H_

#include "ast.h"
#include "visitor.h"


class Parser;
class Addr;
class Evaluator<Addr>;


enum class ParamClass
{
  INTEGER,
  SSE,
  SSEUP,
  X87,
  X87_UP,
  COMPLEX_X87,
  NO_CLASS,
  MEMORY
};


struct ROData
{
  ROData(long ival, int align): ival_(ival), align_(align) {
    label_ = ".LC" + std::to_string(GenTag());
  }

  explicit ROData(const std::string& sval): sval_(sval), align_(1) {
    label_ = ".LC" + std::to_string(GenTag());
  }

  //ROData(const ROData& other) = delete;
  //ROData& operator=(const ROData& other) = delete;


  ~ROData(void) {}

  std::string sval_;
  long ival_;

  int align_;
  std::string label_;

private:
  static long GenTag(void) {
    static long tag = 0;
    return tag++;
  }
};


typedef std::vector<ROData> RODataList;


struct ObjectAddr
{
  ObjectAddr(const std::string& label, const std::string& base, int offset)
      : label_(label), _base(base), offset_(offset) {}

  std::string Repr(void) const;
  
  std::string label_;
  std::string _base;
  int offset_;
  unsigned char bitFieldBegin_ {0};
  unsigned char bitFieldWidth_ {0};
};


struct StaticInitializer
{
  int offset_;
  int width_;
  long val_;
  std::string label_;        
};

typedef std::vector<StaticInitializer> StaticInitList;


class Generator: public Visitor
{
  friend class Evaluator<Addr>;
public:
  Generator(void) {}

  virtual void Visit(ASTNode* node) {
    node->Accept(this);
  }

  void VisitExpr(Expr* expr) {
    expr->Accept(this);
  }

  void VisitStmt(Stmt* stmt) {
    stmt->Accept(this);
  }

  //Expression
  virtual void VisitBinaryOp(BinaryOp* binaryOp);
  virtual void VisitUnaryOp(UnaryOp* unaryOp);
  virtual void VisitConditionalOp(ConditionalOp* condOp);
  virtual void VisitFuncCall(FuncCall* funcCall);
  virtual void VisitObject(Object* obj);
  virtual void VisitEnumerator(Enumerator* enumer);
  virtual void VisitIdentifier(Identifier* ident);
  virtual void VisitConstant(Constant* cons);
  virtual void VisitTempVar(TempVar* tempVar);

  //statement
  virtual void VisitDeclaration(Declaration* init);
  virtual void VisitEmptyStmt(EmptyStmt* emptyStmt);
  virtual void VisitIfStmt(IfStmt* ifStmt);
  virtual void VisitJumpStmt(JumpStmt* jumpStmt);
  virtual void VisitReturnStmt(ReturnStmt* returnStmt);
  virtual void VisitLabelStmt(LabelStmt* labelStmt);
  virtual void VisitCompoundStmt(CompoundStmt* compoundStmt);

  virtual void VisitFuncDef(FuncDef* funcDef);
  virtual void VisitTranslationUnit(TranslationUnit* unit);


  static void SetInOut(Parser* parser, FILE* outFile) {
    parser_ = parser;
    outFile_ = outFile;
  }

  void Gen(void);
  
protected:
  // Binary
  void GenCommaOp(BinaryOp* comma);
  void GenMemberRefOp(BinaryOp* binaryOp);
  //void GenSubScriptingOp(BinaryOp* binaryOp);
  void GenAndOp(BinaryOp* binaryOp);
  void GenOrOp(BinaryOp* binaryOp);
  void GenAddOp(BinaryOp* binaryOp);
  void GenSubOp(BinaryOp* binaryOp);
  void GenAssignOp(BinaryOp* assign);
  void GenCastOp(UnaryOp* cast);
  void GenDerefOp(UnaryOp* deref);
  void GenMinusOp(UnaryOp* minus);
  void GenPointerArithm(BinaryOp* binary);
  void GenDivOp(bool flt, bool sign, int width, int op);
  void GenMulOp(int width, bool flt, bool sign);
  void GenCompOp(int width, bool flt, const char* set);
  void GenCompZero(Type* type);

  // Unary
  void GenPrefixIncDec(Expr* operand, const std::string& inst);
  void GenPostfixIncDec(Expr* operand, const std::string& inst);

  StaticInitializer GetStaticInit(const Initializer& init);

  void GenStaticDecl(Declaration* decl);
  
  void GenSaveArea(void);
  
  void AllocObjects(Scope* scope,
      const FuncDef::ParamList& params=FuncDef::ParamList());

  void CopyStruct(ObjectAddr desAddr, int width);
  
  std::string ConsLabel(Constant* cons);

  std::vector<const char*> GetParamLocation(
      FuncType::TypeList& types, bool retStruct);

  void Emit(const char* format, ...);
  void EmitLabel(const std::string& label);
  void EmitLoad(const std::string& addr, Type* type);
  void EmitLoad(const std::string& addr, int width, bool flt);
  void EmitStore(const std::string& addr, Type* type);
  void EmitStore(const std::string& addr, int width, bool flt);
  void EmitLoadBitField(const std::string& addr, Object* bitField);
  void EmitStoreBitField(const ObjectAddr& addr, Type* type);

  int Push(const char* reg);
  int Pop(const char* reg);

  void Spill(bool flt);

  void Restore(bool flt);

  void Save(bool flt);

  void Exchange(bool flt);

protected:
  static Parser* parser_;
  static FILE* outFile_;

  //static std::string _cons;
  static RODataList rodatas_;
  static int offset_;

  // The address that store the register %rdi,
  //     when the return value is a struct/union
  static int retAddrOffset_;
  static FuncDef* curFunc_;

  static std::vector<Declaration*> staticDecls_;
};


class LValGenerator: public Generator
{
public:
  LValGenerator(void) {}
  
  //Expression
  virtual void VisitBinaryOp(BinaryOp* binaryOp);
  virtual void VisitUnaryOp(UnaryOp* unaryOp);
  virtual void VisitObject(Object* obj);
  virtual void VisitIdentifier(Identifier* ident);

  virtual void VisitConditionalOp(ConditionalOp* condOp) {
    assert(false);
  }
  
  virtual void VisitFuncCall(FuncCall* funcCall) {
    assert(false);
  }

  virtual void VisitEnumerator(Enumerator* enumer) {
    assert(false);
  }

  virtual void VisitConstant(Constant* cons) {
    assert(false);
  }

  virtual void VisitTempVar(TempVar* tempVar);

  ObjectAddr GenExpr(Expr* expr) {
    expr->Accept(this);
    return addr_;
  }
private:
  ObjectAddr addr_ {"", "", 0};
};

#endif
