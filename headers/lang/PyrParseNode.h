/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _PYRPARSENODE_H_
#define _PYRPARSENODE_H_

#include "PyrSlot.h"
#include "PyrKernel.h"
#include "ByteCodeArray.h"
#include "Opcodes.h"
#include "AdvancingAllocPool.h"


enum { rwPrivate=0, rwReadOnly=1, rwWriteOnly=2, rwReadWrite=3 };

enum { varInst, varClass, varTemp, varPseudo };

enum {
	/* structural units */
	pn_ClassNode,
	pn_ClassExtNode,
	pn_MethodNode,
	pn_BlockNode,
	pn_SlotNode,

	/* variable declarations */
	pn_VarListNode,
	pn_VarDefNode,
	pn_DynDictNode,
	pn_DynListNode,
	pn_LitListNode,
	pn_LitDictNode,
	
	pn_StaticVarListNode,
	pn_InstVarListNode,
	pn_PoolVarListNode,
	pn_ArgListNode,
	pn_SlotDefNode,
	
	/* selectors */
	pn_LiteralNode,
	
	/* code */
	pn_PushLitNode,
	pn_PushNameNode,
	pn_PushKeyArgNode,
	pn_CallNode,
	pn_BinopCallNode,
	pn_DropNode,
	pn_AssignNode,
	pn_MultiAssignNode,
	pn_MultiAssignVarListNode,
	pn_SetterNode,
	pn_CurryArgNode,
	
	pn_ReturnNode,
	pn_BlockReturnNode,
	
	pn_NumTypes
};

extern AdvancingAllocPool gParseNodePool;

#define ALLOCNODE(type)  (new (gParseNodePool.Alloc(sizeof(type))) type())
#define ALLOCSLOTNODE(type, classno)  (new (gParseNodePool.Alloc(sizeof(type))) type(classno))
#define COMPILENODE(node, result, onTailBranch) (compileNode((node), (result), (onTailBranch)))
#define DUMPNODE(node, level) do { if (node) (node)->dump(level); } while (false);

struct PyrParseNode {
	PyrParseNode(int classno);
	virtual void compile(PyrSlot *result) = 0;
	virtual void dump(int level) = 0;

	struct PyrParseNode *mNext;
	struct PyrParseNode *mTail;
	short mLineno;
	unsigned char mCharno;
	unsigned char mClassno;
};

struct PyrSlotNode : public PyrParseNode {
	PyrSlotNode() : PyrParseNode(pn_SlotNode) {}
	PyrSlotNode(int classno) : PyrParseNode(classno) {}
	
	virtual void compile(PyrSlot *result);
	virtual void compileLiteral(PyrSlot *result);
	virtual void compilePushLit(PyrSlot *result);
	virtual void dump(int level);
	virtual void dumpLiteral(int level);
	virtual void dumpPushLit(int level);

	PyrSlot mSlot;
};

typedef PyrSlotNode PyrLiteralNode;
typedef PyrSlotNode PyrPushLitNode;
typedef PyrSlotNode PyrPushNameNode;

struct PyrCurryArgNode : public PyrParseNode {
	PyrCurryArgNode() : PyrParseNode(pn_CurryArgNode), mArgNum(-1) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	int mArgNum;
} ;



struct PyrClassExtNode : public PyrParseNode {
	PyrClassExtNode() : PyrParseNode(pn_ClassExtNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode* mClassName;
	struct PyrMethodNode *mMethods;
} ;

struct PyrClassNode : public PyrParseNode {
	PyrClassNode() : PyrParseNode(pn_ClassNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode* mClassName;
	struct PyrSlotNode* mSuperClassName;
	struct PyrSlotNode* mIndexType;
	struct PyrVarListNode *mVarlists;
	struct PyrMethodNode *mMethods;
	int mVarTally[3];
	int mNumSuperInstVars;
} ;

struct PyrMethodNode : public PyrParseNode {
	PyrMethodNode() : PyrParseNode(pn_MethodNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);
	
	struct PyrSlotNode* mMethodName;
	struct PyrSlotNode* mPrimitiveName;
	struct PyrArgListNode *mArglist;
	struct PyrVarListNode *mVarlist;
	struct PyrParseNode *mBody;
	int mIsClassMethod; // is class method?
	bool mExtension;
} ;

struct PyrVarListNode : public PyrParseNode {
	PyrVarListNode() : PyrParseNode(pn_VarListNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrVarDefNode *mVarDefs;
	int mFlags; 
} ;

struct PyrVarDefNode : public PyrParseNode {
	PyrVarDefNode() : PyrParseNode(pn_VarDefNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode* mVarName;
	PyrLiteralNode* mDefVal;
	int mFlags;
} ;

struct PyrCallNodeBase : public PyrParseNode {
	PyrCallNodeBase(int classno) : PyrParseNode(classno) {}

	virtual void compile(PyrSlot *result);
	virtual void compilePartialApplication(int numCurryArgs, PyrSlot *result);
	virtual void compileCall(PyrSlot *result)=0;

	virtual int isPartialApplication()=0;
};

struct PyrCallNodeBase2 : public PyrCallNodeBase {
	PyrCallNodeBase2(int classno) : PyrCallNodeBase(classno) {}
	
	struct PyrSlotNode* mSelector;
	struct PyrParseNode *mArglist;
	struct PyrParseNode *mKeyarglist;
	bool mTailCall;
} ;

struct PyrCallNode : public PyrCallNodeBase2 {
	PyrCallNode() : PyrCallNodeBase2(pn_CallNode) {}

	virtual void compileCall(PyrSlot *result);
	virtual void dump(int level);

	virtual int isPartialApplication();
} ;

struct PyrBinopCallNode : public PyrCallNodeBase2 {
	PyrBinopCallNode() : PyrCallNodeBase2(pn_BinopCallNode) {}

	virtual void compileCall(PyrSlot *result);
	virtual void dump(int level);

	virtual int isPartialApplication();
} ;

struct PyrSetterNode : public PyrCallNodeBase {
	PyrSetterNode() : PyrCallNodeBase(pn_SetterNode) {}
	virtual void compileCall(PyrSlot *result);
	virtual void dump(int level);

	virtual int isPartialApplication();

	struct PyrSlotNode* mSelector;
	struct PyrParseNode *mExpr1;
	struct PyrParseNode *mExpr2;
	int mFlags; // is a var def ?
} ;
	
struct PyrDynListNode : public PyrCallNodeBase {
	PyrDynListNode() : PyrCallNodeBase(pn_DynListNode) {}
	virtual void compileCall(PyrSlot *result);
	virtual void dump(int level);

	virtual int isPartialApplication();

	struct PyrParseNode *mClassname;
	struct PyrParseNode *mElems;
} ;
		
struct PyrDynDictNode : public PyrCallNodeBase {
	PyrDynDictNode() : PyrCallNodeBase(pn_DynDictNode) {}
	virtual void compileCall(PyrSlot *result);
	virtual void dump(int level);

	virtual int isPartialApplication();

	struct PyrParseNode *mElems;
} ;


struct PyrDropNode : public PyrParseNode {
	PyrDropNode() : PyrParseNode(pn_DropNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrParseNode *mExpr1;
	struct PyrParseNode *mExpr2;
} ;

struct PyrPushKeyArgNode : public PyrParseNode {
	PyrPushKeyArgNode() : PyrParseNode(pn_PushKeyArgNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode* mSelector;
	struct PyrParseNode *mExpr;
} ;

struct PyrReturnNode : public PyrParseNode {
	PyrReturnNode() : PyrParseNode(pn_ReturnNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrParseNode *mExpr; // if null, return self
} ;

struct PyrBlockReturnNode : public PyrParseNode {
	PyrBlockReturnNode() : PyrParseNode(pn_BlockReturnNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrParseNode *mExpr; // if null, return self
} ;

struct PyrAssignNode : public PyrParseNode {
	PyrAssignNode() : PyrParseNode(pn_AssignNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode* mVarName;
	struct PyrParseNode *mExpr;
	bool mDrop; // allow drop
} ;
	
struct PyrMultiAssignNode : public PyrParseNode {
	PyrMultiAssignNode() : PyrParseNode(pn_MultiAssignNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrMultiAssignVarListNode *mVarList;
	struct PyrParseNode *mExpr;
	bool mDrop; // allow drop
} ;

struct PyrMultiAssignVarListNode : public PyrParseNode {
	PyrMultiAssignVarListNode() : PyrParseNode(pn_MultiAssignVarListNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrSlotNode *mVarNames;
	struct PyrSlotNode *mRest;
} ;
	
struct PyrBlockNode : public PyrParseNode {
	PyrBlockNode() : PyrParseNode(pn_BlockNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrArgListNode *mArglist;
	struct PyrVarListNode *mVarlist;
	struct PyrParseNode *mBody;
	bool mIsTopLevel;
	int mBeginCharNo;
};

struct PyrArgListNode : public PyrParseNode {
	PyrArgListNode() : PyrParseNode(pn_ArgListNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrVarDefNode *mVarDefs;
	struct PyrSlotNode *mRest;
} ;
	
struct PyrLitListNode : public PyrParseNode {
	PyrLitListNode() : PyrParseNode(pn_LitListNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrParseNode *mClassname;
	struct PyrParseNode *mElems;
} ;
	
struct PyrLitDictNode : public PyrParseNode {
	PyrLitDictNode() : PyrParseNode(pn_LitDictNode) {}
	virtual void compile(PyrSlot *result);
	virtual void dump(int level);

	struct PyrParseNode *mElems;
} ;
	
extern PyrParseNode* gRootParseNode;
extern int gParserResult;
extern bool gIsTailCodeBranch;

extern bool compilingCmdLine;

extern char* nodename[];

void compileNode(PyrParseNode* node, PyrSlot *result, bool onTailBranch);

class SetTailBranch
{
	bool mSave;
public:
	SetTailBranch(bool inValue) { 
		mSave = gIsTailCodeBranch;
		gIsTailCodeBranch = inValue;
	}
	~SetTailBranch() {
		gIsTailCodeBranch = mSave;
	}
};

inline void compileNode(PyrParseNode* node, PyrSlot *result, bool onTailBranch)
{
	SetTailBranch branch(gIsTailCodeBranch && onTailBranch);
		/*if (compilingCmdLine) {
			printf("stb  %14s %d %d\n", nodename[node->mClassno], onTailBranch, gIsTailCodeBranch);
		}*/
	node->compile(result);
}

void initParseNodes();

PyrSlotNode* newPyrSlotNode(PyrSlot *slot);
PyrCurryArgNode* newPyrCurryArgNode();
PyrClassNode* newPyrClassNode(PyrSlotNode* className, PyrSlotNode* superClassName,
	PyrVarListNode* varlists, PyrMethodNode* methods, PyrSlotNode* indexType);
PyrClassExtNode* newPyrClassExtNode(PyrSlotNode* className, PyrMethodNode* methods);
PyrMethodNode* newPyrMethodNode(PyrSlotNode* methodName, PyrSlotNode* primitiveName,
	PyrArgListNode* arglist, PyrVarListNode *varlist, PyrParseNode* body, int isClassMethod);
PyrArgListNode* newPyrArgListNode(PyrVarDefNode* varDefs, PyrSlotNode* rest);
PyrVarListNode* newPyrVarListNode(PyrVarDefNode* vardefs, int flags);
PyrVarDefNode* newPyrVarDefNode(PyrSlotNode* varName, PyrLiteralNode* defVal, int flags);
PyrCallNode* newPyrCallNode(PyrSlotNode* selector, PyrParseNode* arglist,
	PyrParseNode* keyarglist, PyrParseNode* blocklist);
PyrBinopCallNode* newPyrBinopCallNode(PyrSlotNode* selector,
	PyrParseNode* arg1, PyrParseNode* arg2, PyrParseNode* arg3);
PyrDropNode* newPyrDropNode(PyrParseNode* expr1, PyrParseNode* expr2);
PyrPushKeyArgNode* newPyrPushKeyArgNode(PyrSlotNode* selector, PyrParseNode* expr);
PyrPushLitNode* newPyrPushLitNode(PyrSlotNode* literalSlot, PyrParseNode* literalObj);
PyrLiteralNode* newPyrLiteralNode(PyrSlotNode* literalSlot, PyrParseNode* literalObj);
PyrReturnNode* newPyrReturnNode(PyrParseNode* expr);
PyrBlockReturnNode* newPyrBlockReturnNode();
PyrAssignNode* newPyrAssignNode(PyrSlotNode* varName, PyrParseNode* expr, int flags);
PyrSetterNode* newPyrSetterNode(PyrSlotNode* varName, 
	PyrParseNode* expr1, PyrParseNode* expr2);
PyrMultiAssignNode* newPyrMultiAssignNode(PyrMultiAssignVarListNode* varList, 
	PyrParseNode* expr, int flags);
PyrPushNameNode* newPyrPushNameNode(PyrSlotNode *slotNode);
PyrDynDictNode* newPyrDynDictNode(PyrParseNode *elems);
PyrDynListNode* newPyrDynListNode(PyrParseNode *classname, PyrParseNode *elems);
PyrLitListNode* newPyrLitListNode(PyrParseNode *classname, PyrParseNode *elems);
PyrLitDictNode* newPyrLitDictNode(PyrParseNode *elems);
PyrMultiAssignVarListNode* newPyrMultiAssignVarListNode(PyrSlotNode* varNames, 
	PyrSlotNode* rest);
PyrBlockNode* newPyrBlockNode(PyrArgListNode *arglist, PyrVarListNode *varlist, PyrParseNode *body, bool isTopLevel);

void compilePyrMethodNode(PyrMethodNode* node, PyrSlot *result);
void compilePyrLiteralNode(PyrLiteralNode* node, PyrSlot *result);

PyrClass* getNodeSuperclass(PyrClassNode *node);
void countNodeMethods(PyrClassNode* node, int *numClassMethods, int *numInstMethods);
void compileExtNodeMethods(PyrClassExtNode* node);
void countVarDefs(PyrClassNode* node);
bool compareVarDefs(PyrClassNode* node, PyrClass* classobj);
void recompileSubclasses(PyrClass* classobj);
void compileNodeMethods(PyrClassNode* node);
void fillClassPrototypes(PyrClassNode *node, PyrClass *classobj, PyrClass *superclassobj);

int nodeListLength(PyrParseNode *node);
bool isSuperObjNode(PyrParseNode *node);
bool isThisObjNode(PyrParseNode *node);
int conjureSelectorIndex(PyrParseNode *node, PyrBlock* func, 
		bool isSuper, PyrSymbol *selector, int *selType);
int conjureLiteralSlotIndex(PyrParseNode *node, PyrBlock* func, PyrSlot *slot);
bool findVarName(PyrBlock* func, PyrClass **classobj, PyrSymbol *name, 
	int *varType, int *level, int *index, PyrBlock** tempfunc);
void countClassVarDefs(PyrClassNode* node, int *numClassMethods, int *numInstMethods);
void compileNodeList(PyrParseNode *node, bool onTailBranch);
void dumpNodeList(PyrParseNode *node);
int compareCallArgs(PyrMethodNode* node, PyrCallNode *cnode, int *varIndex, PyrClass *specialClass);

bool findSpecialClassName(PyrSymbol *className, int *index);
int getIndexType(PyrClassNode *classnode);

void compileAnyIfMsg(PyrCallNodeBase2* node);
void compileIfMsg(PyrCallNodeBase2* node);
void compileIfNilMsg(PyrCallNodeBase2* node, bool flag);
void compileCaseMsg(PyrCallNodeBase2* node);
void compileWhileMsg(PyrCallNodeBase2* node);
void compileLoopMsg(PyrCallNodeBase2* node);
void compileAndMsg(PyrParseNode* arg1, PyrParseNode* arg2);
void compileOrMsg(PyrParseNode* arg1, PyrParseNode* arg2);
void compileQMsg(PyrParseNode* arg1, PyrParseNode* arg2);
void compileQQMsg(PyrParseNode* arg1, PyrParseNode* arg2);
void compileXQMsg(PyrParseNode* arg1, PyrParseNode* arg2);
void compileSwitchMsg(PyrCallNode* node);

void compilePushInt(int value);
void compileAssignVar(PyrParseNode *node, PyrSymbol* varName, bool drop);
void compilePushVar(PyrParseNode *node, PyrSymbol *varName);
bool isAnInlineableBlock(PyrParseNode *node);
bool isWhileTrue(PyrParseNode *node);
void installByteCodes(PyrBlock *block);

ByteCodes compileSubExpression(PyrPushLitNode* litnode, bool onTailBranch);
ByteCodes compileSubExpressionWithGoto(PyrPushLitNode* litnode, int branchLen, bool onTailBranch);
//ByteCodes compileDefaultValue(int litIndex, int realExprLen);

void initParser();
void finiParser();
void initParserPool();
void freeParserPool();

void initSpecialSelectors();
void initSpecialClasses();

void nodePostErrorLine(PyrParseNode* node);

PyrParseNode* linkNextNode(PyrParseNode* a, PyrParseNode* b);
PyrParseNode* linkAfterHead(PyrParseNode* a, PyrParseNode* b);

extern int compileErrors;

extern long zzval;
extern PyrSymbol *ps_newlist;
extern PyrSymbol *gSpecialUnarySelectors[opNumUnarySelectors];
extern PyrSymbol *gSpecialBinarySelectors[opNumBinarySelectors];
extern PyrSymbol *gSpecialSelectors[opmNumSpecialSelectors];
extern PyrSymbol* gSpecialClasses[op_NumSpecialClasses];

extern PyrClass *gCurrentClass;
extern PyrClass *gCurrentMetaClass;
extern PyrClass *gCompilingClass;
extern PyrMethod *gCompilingMethod;
extern PyrBlock *gCompilingBlock;

/* 
	compiling
	"inlining" of special arithmetic opcodes.
	inlining of IF, WHILE, AND, OR
*/

#endif
