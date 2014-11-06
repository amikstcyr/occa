#include "occaParserStatement.hpp"
#include "occaParser.hpp"

namespace occa {
  namespace parserNamespace {
    //---[ Exp Node ]-------------------------------
    expNode::expNode(statement &s) :
      sInfo(&s),

      value(""),
      info(expType::root),

      up(NULL),

      leafCount(0),
      leaves(NULL),

      varType(NULL),
      varPointerCount(0) {}

    expNode::expNode(expNode &up_) :
      sInfo(up_.sInfo),

      value(""),
      info(expType::root),

      up(&up_),

      leafCount(0),
      leaves(NULL),

      varType(NULL),
      varPointerCount(0) {}

    void expNode::labelStatement(strNode *&nodeRoot){
      if(nodeRoot->type == macroKeywordType)
        sInfo->type = loadMacroStatement(nodeRoot);

      else if(nodeRoot->type == 0){
        sInfo->type = 0;
      }

      else if(nodeRoot->type == keywordType["occaOuterFor0"])
        sInfo->type = loadOccaForStatement(nodeRoot);

      else if(nodeRoot->type & typedefType)
        sInfo->type = loadTypedefStatement(nodeRoot);

      else if(nodeRoot->type & structType)
        sInfo->type = loadStructStatement(nodeRoot);

      else if(nodeRoot->type & (operatorType |
                                presetValue))
        sInfo->type = loadUpdateStatement(nodeRoot);

      else if(sInfo->nodeHasDescriptor(nodeRoot))
        sInfo->type = loadDescriptorStatement(nodeRoot);

      else if(nodeRoot->type & unknownVariable){
        if(nodeRoot->right &&
           nodeRoot->right->value == ":")
          sInfo->type = loadGotoStatement(nodeRoot);

        sInfo->type = loadUpdateStatement(nodeRoot);
      }

      else if(nodeRoot->type & flowControlType)
        sInfo->type = loadFlowStatement(nodeRoot);

      else if(nodeRoot->type & specialKeywordType)
        sInfo->type = loadSpecialStatement(nodeRoot);

      else if((nodeRoot->type == startBrace) &&
              (nodeRoot->up)                 &&
              !(nodeRoot->up->type & operatorType))
        sInfo->type = loadBlockStatement(nodeRoot);

      // Statement: (int) 3;
      else if(nodeRoot->type == emptyType)
        sInfo->type = loadUpdateStatement(nodeRoot);

      else {
        while(nodeRoot &&
              !(nodeRoot->type & endStatement))
          nodeRoot = nodeRoot->right;

        sInfo->type = declareStatementType;
      }
    }

    int expNode::loadMacroStatement(strNode *&nodeRoot){
      info  = expType::macro_;
      value = nodeRoot->value;

      return macroStatementType;
    }

    int expNode::loadOccaForStatement(strNode *&nodeRoot){
      info  = expType::occaFor;
      value = nodeRoot->value;

      return keywordType["occaOuterFor0"];
    }

    int expNode::loadTypedefStatement(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return typedefStatementType;
    }

    int expNode::loadStructStatement(strNode *&nodeRoot){
      if((nodeRoot->value == "struct")       &&
         (nodeRoot->down.size() == 0)        &&
         (nodeRoot->right)                   &&
         (sInfo->hasTypeInScope(nodeRoot->right->value))){

        return sInfo->checkDescriptorStatementType(nodeRoot);
      }

      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return structStatementType;
    }

    int expNode::loadUpdateStatement(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return updateStatementType;
    }

    int expNode::loadDescriptorStatement(strNode *&nodeRoot){
      strNode *oldNodeRoot = nodeRoot;
      strNode *nodePos;

      // Skip descriptors
      while((nodeRoot)                         &&
            (nodeRoot->down.size() == 0)       &&
            ((nodeRoot->type & descriptorType) ||
             sInfo->nodeHasSpecifier(nodeRoot))){

        if(nodeRoot->type & structType){
          nodeRoot = oldNodeRoot;
          return loadStructStatement(nodeRoot);
        }

        nodeRoot = nodeRoot->right;
      }

      nodePos = nodeRoot;

      while((nodeRoot) &&
            !(nodeRoot->type & endStatement))
        nodeRoot = nodeRoot->right;

      const int downCount = nodePos->down.size();

      // Function {Proto, Def | Ptr}
      if(downCount && (nodePos->down[0]->type & parentheses)){
        if(downCount == 1)
          return functionPrototypeType;

        strNode *downNode = nodePos->down[1];

        if(downNode->type & brace){
          nodeRoot = nodePos;
          return functionDefinitionType;
        }

        else if(downNode->type & parentheses){
          downNode = downNode->right;

          if(downNode->type & parentheses){
            std::cout << "Function pointer needs a [*]:\n"
                      << sInfo->prettyString(oldNodeRoot, "  ");
            throw 1;
          }

          while(downNode->value == "*"){
            if(downNode->down.size())
              return declareStatementType;

            downNode = downNode->right;
          }

          // [C++] Function call, not function pointer define
          //    getFunc(0)(arg1, arg2);
          if(sInfo->hasVariableInScope(downNode->value))
            return updateStatementType;

          return declareStatementType;
        }
        else{
          std::cout << "You found the [Waldo 1] error in:\n"
                    << sInfo->prettyString(oldNodeRoot, "  ");
          throw 1;
        }
      }

      return declareStatementType;
    }

    int expNode::loadGotoStatement(strNode *&nodeRoot){
      info  = expType::gotoLabel_;
      value = nodeRoot->value;

      nodeRoot = nodeRoot->right;

      return gotoStatementType;
    }

    int expNode::loadFlowStatement(strNode *&nodeRoot){
      if(nodeRoot->value == "for")
        return forStatementType;
      else if(nodeRoot->value == "while")
        return whileStatementType;
      else if(nodeRoot->value == "do")
        return doWhileStatementType;
      else if(nodeRoot->value == "if")
        return ifStatementType;
      else if(nodeRoot->value == "else if")
        return elseIfStatementType;
      else if(nodeRoot->value == "else")
        return elseStatementType;
      else if(nodeRoot->value == "switch")
        return switchStatementType;

      std::cout << "You found the [Waldo 2] error in:\n"
                << sInfo->prettyString(nodeRoot, "  ");
      throw 1;

      return 0;
    }

    int expNode::loadSpecialStatement(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return blankStatementType;
    }

    int expNode::loadBlockStatement(strNode *&nodeRoot){
      nodeRoot = lastNode(nodeRoot);

      return blockStatementType;
    }

    void expNode::loadFromNode(strNode *&nodePos){
      if(nodePos == NULL){
        sInfo->type = invalidStatementType;
        return;
      }

      strNode *nodeRoot = nodePos;

      labelStatement(nodePos);

      // Don't need to load stuff
      if(sInfo->type & (macroStatementType           |
                       gotoStatementType            |
                       blockStatementType) ||
         (sInfo->type == keywordType["occaOuterFor0"])){

        return;
      }

      //---[ Special Type ]---
      if(nodeRoot->type & specialKeywordType){
        if((nodeRoot->value == "break")    ||
           (nodeRoot->value == "continue") ||
           (nodeRoot->value == "default")){

          value = nodeRoot->value;
          info  = expType::printValue;
          return;
        }

        // [-] Doesn't support GCC's twisted [Labels as Values]
        if(nodeRoot->value == "goto"){
          value = nodeRoot->right->value;
          info  = expType::goto_;
          return;
        }

        // Case where nodeRoot = [case, return]

        if(nodeRoot->value == "case")
          info = expType::case_;
        else if(nodeRoot->value == "return")
          info = expType::return_;
      }
      //======================

      strNode *newNodeRoot = nodeRoot->cloneTo(nodePos);
      strNode *lastNewNode = lastNode(newNodeRoot);

      if(lastNewNode == NULL)
        newNodeRoot->print();

      //---[ Extra Blocks ]---
      if(lastNewNode->down.size()){
        int downsAvailable = 0;

        if(sInfo->type & (forStatementType      |
                          switchStatementType   |
                          functionStatementType)){

          downsAvailable = 1;
        }
        // Skip do
        else if(sInfo->type == whileStatementType){
          downsAvailable = 1;
        }
        // Only [if] and [if else]
        else if((sInfo->type & ifStatementType) &&
                (sInfo->type != elseStatementType)){
          downsAvailable = 1;
        }

        lastNewNode->down.erase(lastNewNode->down.begin() + downsAvailable,
                                lastNewNode->down.end());
      }
      //======================

      splitAndOrganizeNode(newNodeRoot);

      // Only the root needs to free
      if(up == NULL)
        occa::parserNamespace::free(newNodeRoot);
    }

    void expNode::splitAndOrganizeNode(strNode *nodeRoot){
      addNewVariables(nodeRoot);
      initLoadFromNode(nodeRoot);

      if(sInfo->type & declareStatementType)
        splitDeclareStatement();

      else if(sInfo->type & forStatementType)
        splitForStatement();

      else if(sInfo->type & functionStatementType)
        splitFunctionStatement();

      else if(sInfo->type & structStatementType)
        splitStructStatement();

      else if(sInfo->type & typedefStatementType)
        splitTypedefStatement();

      else
        organize();

      // [-] Need to fix typedef output
      // print();
      // std::cout << "this = " << *this << '\n';

      updateNewVariables();
    }

    void expNode::organize(){
      if(leafCount == 0)
        return;

      initOrganization();
      organizeLeaves();
    }

    // Disregards function pointers, those are "easy"
    void expNode::addNewVariables(strNode *nodePos){
      if((up != NULL) ||
         !(sInfo->type & (declareStatementType |
                          functionStatementType)) )
        return;

      strNode *lastPos = NULL;

      if(sInfo->type & functionStatementType){
        nodePos = lastNode(nodePos);

        // Function Definition: void f(){}
        // Function Prototype : void f()[;]
        //                   Extra node _^
        if(sInfo->type & functionPrototypeType)
          nodePos = nodePos->left;

        //---[ Add Function ]-----------
        varInfo newVar;
        newVar.name = nodePos->value;

        if( !(sInfo->up->hasVariableInScope(newVar.name)) )
          sInfo->up->addVariable(newVar);
        //==============================

        nodePos = nodePos->down[0];
        lastPos = lastNode(nodePos);
        lastPos->value = ";";

        if(nodePos->down.size() == 0)
          nodePos = nodePos->right;
      }

      bool loadingFunctionPointer;

      while(nodePos){
        // Skip Type
        while((nodePos) &&
              (nodePos->type & qualifierType)){

          nodePos = nodePos->right;
        }

        if(sInfo->nodeHasSpecifier(nodePos))
          nodePos = nodePos->right;

        if((nodePos->down.size() == 0) ||
           (nodePos->down[0]->type != startParentheses)){

          loadingFunctionPointer = false;
        }
        else{
          loadingFunctionPointer = true;
        }

        varInfo newVar;

        if(!loadingFunctionPointer){
          // Mark pointer qualifiers
          while((nodePos) &&
                (nodePos->type & qualifierType)){

            nodePos = nodePos->right;
          }

          const int downCount = nodePos->down.size();

          // Not a function pointer
          if((downCount == 0) ||
             (nodePos->down[0]->type != startParentheses)){

            newVar.name = nodePos->value;

            for(int i = 0; i < downCount; ++i){
              if(nodePos->down[i]->type == startBracket){
                strNode *leftBracket  = nodePos->down[i];
                strNode *rightBracket = lastNode(leftBracket);

                if(leftBracket->right != rightBracket){
                  strNode *lastDown = rightBracket->left;
                  lastDown->right = NULL;

                  newVar.stackPointerSizes.push_back((std::string) *(leftBracket->right));

                  lastDown->right = rightBracket;
                }
                else
                  newVar.stackPointerSizes.push_back("");
              }
              else
                break;
            }

            // Add new variables
            sInfo->addVariable(newVar);

            nodePos = nodePos->right;
          }
        }

        // int [(*f)()], [(*g)()]
        else{
          strNode *downNode = nodePos->down[0];
          downNode = downNode->right;

          if((downNode) &&
             (downNode->value == "*")){

            while((downNode) &&
                  (downNode->type & qualifierType)){

              downNode = downNode->right;
            }

            newVar.name = downNode->value;

            // [-] Not fully true
            sInfo->addVariable(newVar);
          }
        }

        // Go to [,] or [;]
        while((nodePos)               &&
              (nodePos->value != ",") &&
              (nodePos->value != ";")){

          nodePos = nodePos->right;
        }

        if(nodePos){
          loadingFunctionPointer = (nodePos->down.size() != 0);

          nodePos = nodePos->right;
        }
      }

      if(lastPos)
        lastPos->value = ")";
    }

    void expNode::updateNewVariables(){
      if((up != NULL) ||
         !(sInfo->type & (declareStatementType |
                          functionDefinitionType)) )
        return;

      if(sInfo->type & declareStatementType){
        expNode &sType     = *(sInfo->getDeclarationTypeNode());
        const int varCount = sInfo->getDeclarationVarCount();

        for(int i = 0; i < varCount; ++i){
          expNode &varNode = *(sInfo->getDeclarationVarNode(i));

          std::string varName = sInfo->getDeclarationVarName(i);
          varInfo &var        = *(sInfo->scopeVarMap[varName]);

          sType.setVarInfo(var);

          if(varNode.leaves[0]->info & expType::qualifier)
            varNode.leaves[0]->setVarInfo(var);
        }
      }
      else if(sInfo->type & functionDefinitionType){
        varInfo &var = *(sInfo->up->scopeVarMap[ leaves[1]->value ]);

        var.typeInfo |= functionType;

        leaves[0]->setVarInfo(var);

        const int argc = sInfo->getFunctionArgCount();

        for(int i = 0; i < argc; ++i){
          expNode &arg = *(sInfo->getFunctionArgNode(i));

          if(arg.info & expType::variable){
            varInfo &sVar = *(sInfo->scopeVarMap[ sInfo->getFunctionArgName(i) ]);

            // [const int] x[2]
            arg.leaves[0]->setVarInfo(sVar);

            // const int x[[2]]
            if(arg.leafCount == 3)
              arg.leaves[2]->setVarInfo(sVar);
          }
        }
      }

      // print();
    }

    void expNode::splitDeclareStatement(){
      info = expType::declaration;

      if(leafCount == 0)
        return;

      if(leaves[leafCount - 1]->value != ";"){
        addNode(expType::printValue, leafCount);
        leaves[leafCount - 1]->value = ";";
      }

      const bool hasType = (leaves[0]->info & (expType::qualifier |
                                               expType::type));

      int varCount = hasType;

      for(int i = 0; i < leafCount; ++i){
        if((leaves[i]->info & expType::operator_) &&
           (leaves[i]->value == ",")){

          ++varCount;
        }
      }

      expNode **newLeaves = new expNode*[1 + varCount];

      int first = 0;
      varCount  = 0;

      for(int i = 0; i < leafCount; ++i){
        const bool addingType = (hasType && (first == 0));

        const bool delimeterStop = (!addingType && ((leaves[i]->value == ",") ||
                                                    (leaves[i]->value == ";")));

        const bool addingTypeStop = (addingType && !(leaves[i]->info & (expType::qualifier |
                                                                        expType::type)));

        if(delimeterStop | addingTypeStop){
          expNode *newLeaf = new expNode(*this);
          const int newLeafCount = (i - first);

          newLeaf->info      = expType::root;
          newLeaf->leafCount = newLeafCount;
          newLeaf->leaves    = new expNode*[newLeafCount];

          for(int i = 0; i < newLeafCount; ++i)
            newLeaf->leaves[i] = leaves[first + i];

          newLeaf->organize();

          if(delimeterStop){
            delete leaves[i];

            first = (i + 1);

            newLeaves[varCount++] = newLeaf;
          }
          else{
            first = i;

            newLeaves[varCount++] = newLeaf->leaves[0];

            delete [] newLeaf->leaves;
            delete newLeaf;
          }
        }
      }

      delete [] leaves;

      leaves    = newLeaves;
      leafCount = varCount;
    }

    void expNode::splitForStatement(){
      info = expType::checkSInfo;

      // [+]
      // print();

      if((leafCount < 2) ||
         (leaves[1]->value != "(")){
        std::cout << "Wrong syntax for [for]:\n";
        print();
        throw 1;
      }

      expNode **sLeaves    = leaves[1]->leaves;
      const int sLeafCount = leaves[1]->leafCount;

      delete leaves[0];
      delete leaves[1];

      leafCount = sLeafCount;
      leaves    = sLeaves;

      int statementCount = 1;

      for(int i = 0; i < leafCount; ++i){
        if(leaves[i]->value == ";")
          ++statementCount;
      }

      sLeaves = new expNode*[statementCount];

      int firstLeaf  = 0;
      statementCount = 0;

      for(int i = 0; i <= leafCount; ++i){
        if((i == leafCount) ||
           (leaves[i]->value == ";")){

          expNode &ssLeaf = *(new expNode(*this));

          if(firstLeaf != i){
            ssLeaf.leafCount = (i - firstLeaf);
            ssLeaf.leaves    = new expNode*[ssLeaf.leafCount];

            for(int j = firstLeaf; j < i; ++j)
              ssLeaf.leaves[j - firstLeaf] = leaves[j];
          }

          if(i < leafCount)
            delete leaves[i];

          if(i != 0)
            ssLeaf.organize();
          else
            ssLeaf.splitDeclareStatement();

          sLeaves[statementCount++] = &ssLeaf;
          firstLeaf = (i + 1);
        }
      }

      leaves    = sLeaves;
      leafCount = statementCount;
    }

    void expNode::splitFunctionStatement(){
      if(sInfo->type & functionDefinitionType)
        info = (expType::function | expType::declaration);
      else
        info = (expType::function | expType::prototype);

      int argPos = 0;
      int argc   = 0;
      int pos    = 0;

      for(argPos = 0; argPos < leafCount; ++argPos){
        if(leaves[argPos]->info == expType::C)
          break;
      }

      if(argPos){
        int typePos = (argPos - 1);

        const int trueLeafCount = leafCount;
        leafCount = typePos;

        organize();

        const int newArgPos = leafCount;

        for(int i = typePos; i < trueLeafCount; ++i)
          leaves[newArgPos + i - typePos] = leaves[i];

        leafCount = trueLeafCount - (typePos - newArgPos);
        argPos    = (newArgPos + 1);
      }

      expNode &argNode = *(leaves[argPos]);

      // [-] Remove [;] on prototypes
      if((argPos + 1) < leafCount)
        leafCount = argPos + 1;

      if(argNode.leafCount){
        argc = 1;

        for(int i = 0; i < argNode.leafCount; ++i){
          if(argNode.leaves[i]->value == ",")
            ++argc;
        }

        expNode **sLeaves = new expNode*[argc];
        argc = 0;

        for(int i = 0; i <= argNode.leafCount; ++i){
          if((i == argNode.leafCount) ||
             (argNode.leaves[i]->value == ",")){

            expNode &sLeaf = *(new expNode(argNode));
            sLeaves[argc]  = &sLeaf;

            sLeaf.leafCount = (i - pos);
            sLeaf.leaves    = new expNode*[sLeaf.leafCount];

            for(int j = pos; j < i; ++j)
              sLeaf.leaves[j - pos] = argNode.leaves[j];

            sLeaf.organize();

            expNode *ssLeaf = sLeaf.leaves[0];
            delete &sLeaf;

            sLeaves[argc] = ssLeaf;
            ssLeaf->up    = sLeaves[argc];

            ++argc;
            pos = (i + 1);
          }
        }

        delete [] argNode.leaves;

        argNode.leaves    = sLeaves;
        argNode.leafCount = argc;
      }
    }

    void expNode::splitStructStatement(){
      info = expType::struct_;

      int pos = 0;

      //---[ Merge qualifiers ]---------
      for(pos = 0; pos < leafCount; ++pos){
        if( !(leaves[pos]->info & expType::qualifier) )
          break;
      }

      if(pos){
        expNode *sNewLeaf = new expNode(*this);

        sNewLeaf->info      = expType::qualifier;
        sNewLeaf->leafCount = pos;
        sNewLeaf->leaves    = new expNode*[pos];

        for(int i = 0; i < pos; ++i){
          sNewLeaf->leaves[i] = leaves[i];
          leaves[i]->up       = sNewLeaf;
        }

        leaves[0] = sNewLeaf;

        for(int i = pos; i < leafCount; ++i)
          leaves[i - pos + 1] = leaves[i];

        leafCount -= (pos - 1);
      }
      //================================

      for(pos = 0; pos < leafCount; ++pos){
        if((leaves[pos]->info == expType::C) ||
           (leaves[pos]->value == ";")) // Empty struct
          break;
      }

      if((pos)              &&
         (pos != leafCount) &&
         !(leaves[pos - 1]->info & expType::qualifier)){

        sInfo->up->addTypeDef(leaves[pos - 1]->value);
      }

      leaves[pos]->splitStructStatements();

      // Skip {}
      ++pos;

      // Return if pos is at [;]
      if(pos == (leafCount - 1)){
        --leafCount; // Remove [;]
        return;
      }

      expNode *newLeaf = new expNode(*this);
      const int newLeafCount = (leafCount - pos);

      newLeaf->up        = this;
      newLeaf->info      = expType::root;
      newLeaf->leafCount = newLeafCount;
      newLeaf->leaves    = new expNode*[newLeafCount];

      for(int i = 0; i < newLeafCount; ++i)
        newLeaf->leaves[i] = leaves[pos + i];

      newLeaf->splitDeclareStatement();

      leaves[pos] = newLeaf;
      leafCount = (pos + 1);
    }

    void expNode::splitStructStatements(){
      int extras = 0;

      for(int i = 0; i < leafCount; ++i){
        if(leaves[i]->value == ";")
          ++extras;
      }

      if(extras == 0)
        return;

      expNode **newLeaves = new expNode*[extras];
      extras = 0;

      int first = 0;

      for(int i = 0; i < leafCount; ++i){
        if(leaves[i]->value == ";"){
          bool newLeafIsAStruct = false;

          expNode *newLeaf = new expNode(*this);
          const int newLeafCount = (i - first + 1);

          newLeaf->up        = this;
          newLeaf->info      = expType::root;
          newLeaf->leafCount = newLeafCount;
          newLeaf->leaves    = new expNode*[newLeafCount];

          for(int i = 0; i < newLeafCount; ++i){
            newLeaf->leaves[i] = leaves[first + i];

            if((newLeaf->leaves[i]->info & expType::qualifier) &&
               (newLeaf->leaves[i]->value == "struct")         &&
               (  !(((i + 1) < newLeafCount) &&
                    (sInfo->hasTypeInScope(leaves[first + i + 1]->value)))  )){

              newLeafIsAStruct = true;
            }
          }

          if(!newLeafIsAStruct)
            newLeaf->splitDeclareStatement();
          else
            newLeaf->splitStructStatement();

          first = i + 1;
          newLeaves[extras++] = newLeaf;
        }
      }

      delete [] leaves;

      leaves    = newLeaves;
      leafCount = extras;
    }

    void expNode::splitTypedefStatement(){
      --leafCount;

      bool splitStruct = false;

      for(int i = 0; i < leafCount; ++i){
        leaves[i] = leaves[i + 1];

        if((leaves[i]->info & expType::qualifier) &&
           ((leaves[i]->value == "struct") ||
            (leaves[i]->value == "union") ||
            (leaves[i]->value == "enum"))){

          // Make sure struct is not being used as a qualifier
          if(  !((leaves[i]->value == "struct") &&
                 ((i + 2) < leafCount)          &&
                 (sInfo->hasTypeInScope(leaves[i + 2]->value)))  ){

            splitStruct = true;
          }
        }
      }

      if(!splitStruct)
        splitDeclareStatement();
      else
        splitStructStatement();

      expNode *sLastLeaf = lastLeaf()->lastLeaf();

      if(!splitStruct){
        sInfo->up->addTypeDef(sLastLeaf->value);
      }
      else{
        expNode *ssLastLeaf = sLastLeaf->lastLeaf();
        sInfo->up->addTypeDef(ssLastLeaf->value);
      }

      info |= expType::typedef_;
    }

    void expNode::initLoadFromNode(strNode *nodeRoot,
                                   const int initPos){
      strNode *nodePos = nodeRoot;

      // Root
      if(nodePos->type == 0){
        leafCount += nodePos->down.size();
        nodePos = nodePos->right;
      }

      while(nodePos){
        leafCount += (1 + nodePos->down.size());
        nodePos = nodePos->right;
      }

      if(leafCount == 0)
        return;

      nodePos = nodeRoot;

      int leafPos;

      if(initPos){
        expNode **sLeaves = new expNode*[leafCount];

        for(int i = 0; i < initPos; ++i)
          sLeaves[i] = leaves[i];

        delete [] leaves;

        leaves  = sLeaves;
        leafPos = initPos;
      }
      else{
        leaves = new expNode*[leafCount];
        leafPos = 0;
      }

      while(nodePos){
        if(nodePos->type == 0){
          leafPos = initDownsFromNode(nodePos, leafPos);
          nodePos = nodePos->right;

          continue;
        }

        expNode *&leaf = leaves[leafPos++];

        leaf        = new expNode(*this);
        leaf->up    = this;
        leaf->value = nodePos->value;

        if(nodePos->type & unknownVariable){
          varInfo *nodeVar = sInfo->hasVariableInScope(nodePos->value);

          if(nodeVar){
            if( !(nodeVar->typeInfo & functionType) )
              leaf->info = expType::variable;
            else{
              if( !(sInfo->type & functionStatementType) )
                sInfo->varUsedMap[nodeVar].push(sInfo);

              leaf->info = expType::function;
            }
          }
          else{
            typeDef *nodeType = sInfo->hasTypeInScope(nodePos->value);

            if(!nodeType)
              leaf->info = expType::unknown;
            else
              leaf->info = expType::type;
          }
        }

        else if(nodePos->type & presetValue){
          leaf->info = expType::presetValue;
        }

        else if(nodePos->type & descriptorType){
          if(nodePos->type & (qualifierType | structType))
            leaf->info = expType::qualifier;
          else
            leaf->info = expType::type;

          // For [*] and [&]
          if(nodePos->type & operatorType)
            leaf->info |= expType::operator_;
        }

        else if(nodePos->type & structType){
          leaf->info = expType::qualifier;
        }

        else if(nodePos->type & operatorType){
          leaf->info = expType::operator_;
        }

        else
          leaf->info = expType::printValue;

        if(nodePos->type == 0){
          delete leaf;
          --leafPos;
        }

        leafPos = initDownsFromNode(nodePos, leafPos);

        nodePos = nodePos->right;
      }
    }

    int expNode::initDownsFromNode(strNode *nodePos, int leafPos){
      const int downCount = nodePos->down.size();

      if(downCount == 0)
        return leafPos;

      if(leafCount == 0){
        leafCount = downCount;
        leaves    = new expNode*[leafCount];
      }

      for(int i = 0; i < downCount; ++i){
        strNode *downNode = nodePos->down[i];
        strNode *lastDown = lastNode(downNode);

        popAndGoLeft(lastDown);

        std::string sValue = downNode->value;

        expNode *&sLeaf = leaves[leafPos++];

        sLeaf        = new expNode(*this);
        sLeaf->value = sValue;
        sLeaf->info  = expType::C;

        int sLeafCount = 0;

        // (downNode == lastDown) if loading [()]
        if(downNode->down.size())
          sLeafCount = sLeaf->initDownsFromNode(downNode);

        if(downNode != lastDown){
          // Don't pop (root's free will screw up)
          sLeaf->initLoadFromNode(downNode->right, sLeafCount);
        }
      }

      return leafPos;
    }

    void expNode::initOrganization(){
      // Init ()'s bottom -> up
      for(int i = 0; i < leafCount; ++i){
        if(leaves[i]->leafCount)
          leaves[i]->initOrganization();
      }

      //---[ Level 0 ]------
      // [a][::][b]
      mergeNamespaces();

      // [(class)]
      labelCasts();

      // const int [*] x
      labelReferenceQualifiers();

      // [const] int [*] x
      mergeQualifiers();

      // [[const] [int] [*]] x
      mergeTypes();

      // [qualifiers] [type] (*[name]) ([args])
      mergeFunctionPointers();

      if(sInfo->type & declareStatementType){
        labelNewVariables();
      }
      else{
        // [[[const] [int] [*]] [x]]
        mergeVariables();
      }
      //====================
    }

    void expNode::organizeLeaves(){
      // Organize leaves bottom -> up
      for(int i = 0; i < leafCount; ++i){
        if((leaves[i]->leafCount) &&
           !(leaves[i]->info & (expType::type      |
                                expType::qualifier |
                                expType::function  |
                                expType::functionPointer))){

          leaves[i]->organizeLeaves();
        }
      }

      //---[ Level 1 ]------
      // class(...), class{1,2,3}
      mergeClassConstructs();

      // static_cast<>()
      mergeCasts();

      // [max(a,b)]
      mergeFunctionCalls();

      // a[3]
      mergeArrays();

      organizeLeaves(1);
      //====================

      //---[ Level 2 ]------
      // (class) x
      mergeClassCasts();

      // sizeof x
      mergeSizeOf();

      // new, new [], delete, delete []
      mergeNewsAndDeletes();

      organizeLeaves(2);
      //====================

      //---[ Level 3-14 ]---
      for(int i = 3; i <= 14; ++i)
        organizeLeaves(i);
      //====================

      //---[ Level 15 ]-----
      // throw x
      mergeThrows();
      //====================

      //---[ Level 16 ]-----
      organizeLeaves(16);
      //====================
    }

    void expNode::organizeLeaves(const int level){
      int leafPos = 0;

      while(leafPos < leafCount){
        if(leaves[leafPos]->leafCount){
          ++leafPos;
          continue;
        }

        opLevelMapIterator it = opLevelMap[level].find(leaves[leafPos]->value);

        if(it == opLevelMap[level].end()){
          ++leafPos;
          continue;
        }

        const int levelType = it->second;

        if(levelType & unitaryOperatorType){
          bool updateNow = true;

          // Cases:  1 + [-]1
          //         (+1)
          if((leaves[leafPos]->value.size() == 1) &&
             ((leaves[leafPos]->value[0] == '+') ||
              (leaves[leafPos]->value[0] == '-'))){

            if(leafPos &&
               !(leaves[leafPos - 1]->info & expType::operator_))
              updateNow = false;
          }

          if(updateNow){
            int target = leafPos + ((levelType & lUnitaryOperatorType) ?
                                    1 : -1);

            if((target < 0) || (leafCount <= target)){
              ++leafPos;
            }
            else{
              if(levelType & lUnitaryOperatorType)
                leafPos = mergeLeftUnary(leafPos);
              else
                leafPos = mergeRightUnary(leafPos);
            }
          }
          else
            ++leafPos;
        }
        else if(levelType & binaryOperatorType)
          leafPos = mergeBinary(leafPos);
        else if(levelType & ternaryOperatorType)
          leafPos = mergeTernary(leafPos);
        else
          ++leafPos;
      }
    }

    int expNode::mergeRange(const int newLeafType,
                            const int leafPosStart,
                            const int leafPosEnd){
      expNode *newLeaf = new expNode(*this);

      newLeaf->up        = this;
      newLeaf->info      = newLeafType;
      newLeaf->leafCount = (leafPosEnd - leafPosStart + 1);
      newLeaf->leaves    = new expNode*[newLeaf->leafCount];

      for(int i = 0; i < newLeaf->leafCount; ++i){
        newLeaf->leaves[i]     = leaves[leafPosStart + i];
        newLeaf->leaves[i]->up = newLeaf;
      }

      leaves[leafPosStart] = newLeaf;

      for(int i = (leafPosEnd + 1); i < leafCount; ++i)
        leaves[leafPosStart + i - leafPosEnd] = leaves[i];

      leafCount -= (newLeaf->leafCount - 1);

      return (leafPosStart + 1);
    }

    // [a][::][b]
    void expNode::mergeNamespaces(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if(leaves[leafPos]->value == "::"){
          // [-] Change after keeping track of namespaces
          int leafPosStart = leafPos - (leafPos &&
                                        (leaves[leafPos - 1]->info & expType::unknown));
          // int leafPosStart = leafPos - (leafPos &&
          //                               (leaves[leafPos - 1]->info & expType::namespace_));
          int leafPosEnd   = leafPos + 2;

          while((leafPosEnd < leafCount) &&
                (leaves[leafPosEnd]->value == "::"))
            leafPosEnd += 2;

          --leafPosEnd;

          leafPos = mergeRange(expType::type | expType::namespace_,
                               leafPosStart, leafPosEnd);
        }
        else
          ++leafPos;
      }
    }

    // [(class)]
    void expNode::labelCasts(){
      // Don't mistake:
      //   int main(int) -> int main[(int)]
      if(sInfo->type & functionStatementType)
        return;

      int leafPos = 0;

      while(leafPos < leafCount){
        if((leaves[leafPos]->info == expType::C) &&
           (leaves[leafPos]->value == "(")       &&
           (leaves[leafPos]->leafCount)          &&
           (leaves[leafPos]->leaves[0]->info & expType::type)){

          leaves[leafPos]->info |= expType::cast_;
        }

        ++leafPos;
      }
    }

    // const int [*] x
    void expNode::labelReferenceQualifiers(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if(leaves[leafPos]->info == (expType::operator_ |
                                     expType::qualifier)){
          if(leafPos){

            if(leaves[leafPos - 1]->info & expType::qualifier){
              leaves[leafPos]->info = expType::qualifier;
            }

            else if(leaves[leafPos - 1]->info & expType::unknown){
              if(!sInfo->hasTypeInScope(leaves[leafPos - 1]->value))
                leaves[leafPos]->info = expType::operator_;
              else
                leaves[leafPos]->info = expType::qualifier;
            }

            else if(leaves[leafPos - 1]->info & (expType::L           |
                                                 expType::R           |
                                                 expType::presetValue |
                                                 expType::variable    |
                                                 expType::function)){

              leaves[leafPos]->info = expType::operator_;
            }

            else if((leaves[leafPos - 1]->info & expType::C) &&
                    ~(leaves[leafPos - 1]->info & expType::cast_)){

              leaves[leafPos]->info = expType::operator_;
            }

            else{
              leaves[leafPos]->info = expType::qualifier;
            }
          }
          else{
            leaves[leafPos]->info = expType::qualifier;
          }
        }

        ++leafPos;
      }
    }

    // [const] int x
    void expNode::mergeQualifiers(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if(leaves[leafPos]->info & expType::qualifier){
          int leafPosStart = leafPos;
          int leafPosEnd   = leafPos;

          while((leafPosEnd < leafCount) &&
                (leaves[leafPosEnd]->info & expType::qualifier))
            ++leafPosEnd;

          leafPos = mergeRange(expType::qualifier,
                               leafPosStart, leafPosEnd - 1);
        }
        else
          ++leafPos;
      }
    }

    // [[const] [int] [*]] x
    void expNode::mergeTypes(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if(leaves[leafPos]->info & expType::type){
          const bool varHasLQualifiers = (leafPos &&
                                          (leaves[leafPos - 1]->info & expType::qualifier));

          const bool varHasRQualifiers = (((leafPos + 1) < leafCount) &&
                                          (leaves[leafPos + 1]->info & expType::qualifier));

          expNode &typeNode = *(leaves[leafPos - varHasLQualifiers]);

          typeNode.varType  = new varInfo;
          varInfo &var      = *(typeNode.varType);

          if(varHasLQualifiers){
            expNode &lqNode = *(leaves[leafPos - 1]);

            var.descriptors.resize(lqNode.leafCount);

            for(int i = 0; i < lqNode.leafCount; ++i)
              var.descriptors[i] = lqNode.leaves[i]->value;
          }

          var.type = sInfo->hasTypeInScope(leaves[leafPos]->value);
          var.pointerCount = 0;

          if(varHasRQualifiers){
            expNode &rqNode = *(leaves[leafPos + 1]);

            var.descriptors.resize(rqNode.leafCount);

            for(int i = 0; i < rqNode.leafCount; ++i){
              if(rqNode.leaves[i]->value == "*")
                ++(var.pointerCount);
            }

            typeNode.varPointerCount = var.pointerCount;
          }

          leafPos = mergeRange(expType::type,
                               leafPos - varHasLQualifiers,
                               leafPos + varHasRQualifiers);
        }
        else
          ++leafPos;
      }
    }

    // [[[const] [int] [*]] [x]]
    void expNode::mergeVariables(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if((leaves[leafPos]->info & expType::type) && // [[const] [int] [*]]
           ((leafPos + 1) < leafCount)             && // [x]
           (leaves[leafPos + 1]->info & expType::variable)){

          leafPos = mergeRange(expType::variable,
                               leafPos, leafPos + 1);
        }
        else
          ++leafPos;
      }
    }

    // 1 [type]                           2 [(]       3 [(]
    // [[qualifiers] [type] [qualifiers]] [(*[name])] [([args])]
    void expNode::mergeFunctionPointers(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if((leaves[leafPos]->info & expType::type)   &&     // 1
           ((leafPos + 2) < leafCount)               &&     // Has 2 & 3
           (leaves[leafPos + 1]->info == expType::C) &&     // 2
           (leaves[leafPos + 2]->info == expType::C) &&     // 3
           (leaves[leafPos + 1]->leaves[0]->value == "*")){ // 2.5

          expNode *newLeaf = new expNode(*this);

          newLeaf->up        = this;
          newLeaf->info      = expType::functionPointer;
          newLeaf->leafCount = 3;
          newLeaf->leaves    = new expNode*[3];
          newLeaf->leaves[0] = leaves[leafPos];
          newLeaf->leaves[1] = leaves[leafPos + 1]->leaves[1];
          newLeaf->leaves[2] = leaves[leafPos + 2];

          // Merge arguments in [leafPos + 2]
          newLeaf->leaves[2]->mergeArguments();

          // Don't kill the name of the function pointer
          leaves[leafPos + 1]->leafCount = 1;
          freeLeaf(leafPos + 1);

          leaves[leafPos] = newLeaf;

          for(int i = (leafPos + 3); i < leafCount; ++i)
            leaves[i - 2] = leaves[i];

          ++leafPos;
          leafCount -= 2;
        }
        else
          ++leafPos;
      }
    }

    // class(...), class{1,2,3}
    void expNode::mergeClassConstructs(){
    }

    // static_cast<>()
    void expNode::mergeCasts(){
    }

    // [max(a,b)]
    void expNode::mergeFunctionCalls(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if((leaves[leafPos]->info  == expType::C) &&
           (leaves[leafPos]->value == "(")){

          if((leafPos) &&
             (leaves[leafPos - 1]->info & expType::function)){
            expNode &fNode    = *(leaves[leafPos - 1]);
            expNode &argsNode = *(leaves[leafPos    ]);

            fNode.addNode(argsNode.info);

            delete fNode.leaves[0];
            fNode.leaves[0] = &argsNode;

            removeNode(leafPos);
            --leafPos;
          }
        }

        ++leafPos;
      }
    }

    void expNode::mergeArguments(){
      for(int i = 0; i < leafCount; i += 2){
        leaves[i/2] = leaves[i];

        if((i + 1) < leafCount)
          freeLeaf(i + 1);
      }

      leafCount = ((leafCount / 2) + 1);
    }

    // a[2]
    void expNode::mergeArrays(){
      int leafPos = 0;

      while(leafPos < leafCount){
        if((leaves[leafPos]->info & expType::C) &&
           (leaves[leafPos]->value == "[")){

          int brackets = 0;

          while(((leafPos + brackets) < leafCount) &&
                (leaves[leafPos + brackets]->info & expType::C) &&
                (leaves[leafPos + brackets]->value == "[")){

            ++brackets;
          }

          const bool inserting = ((leaves[leafPos - 1]->info & expType::variable) &&
                                  leaves[leafPos - 1]->leafCount);

          expNode *newLeaf;

          if(inserting){
            newLeaf = leaves[leafPos - 1];
          }
          else{
            newLeaf = new expNode(*this);

            newLeaf->up        = this;
            newLeaf->info      = expType::variable;
            newLeaf->leafCount = 2;
            newLeaf->leaves    = new expNode*[2];
          }

          expNode *sNewLeaf = new expNode(*newLeaf);

          sNewLeaf->up        = newLeaf;
          sNewLeaf->info      = expType::qualifier;
          sNewLeaf->leafCount = brackets;
          sNewLeaf->leaves    = new expNode*[brackets];

          if(inserting){
            newLeaf->addNode(expType::qualifier, newLeaf->leafCount);
            newLeaf->leaves[newLeaf->leafCount - 1] = sNewLeaf;
          }
          else{
            newLeaf->leaves[0] = leaves[leafPos - 1];
            newLeaf->leaves[1] = sNewLeaf;

            leaves[leafPos - 1] = newLeaf;
          }

          for(int i = 0; i < brackets; ++i)
            sNewLeaf->leaves[i] = leaves[leafPos + i];

          for(int i = (leafPos + brackets); i < leafCount; ++i)
            leaves[i - brackets] = leaves[i];

          ++leafPos;
          leafCount -= brackets;
        }
        else
          ++leafPos;
      }
    }

    // (class) x
    void expNode::mergeClassCasts(){
    }

    // sizeof x
    void expNode::mergeSizeOf(){
    }

    // new, new [], delete, delete []
    void expNode::mergeNewsAndDeletes(){
    }

    // throw x
    void expNode::mergeThrows(){
    }

    // [++]i
    int expNode::mergeLeftUnary(const int leafPos){
      expNode *leaf  = leaves[leafPos];
      expNode *sLeaf = leaves[leafPos + 1];

      for(int i = (leafPos + 2); i < leafCount; ++i)
        leaves[i - 1] = leaves[i];

      --leafCount;

      leaf->info      = expType::L;
      leaf->leafCount = 1;
      leaf->leaves    = new expNode*[1];
      leaf->leaves[0] = sLeaf;

      sLeaf->up = leaf;

      return (leafPos + 1);
    }

    // i[++]
    int expNode::mergeRightUnary(const int leafPos){
      expNode *leaf  = leaves[leafPos];
      expNode *sLeaf = leaves[leafPos - 1];

      leaves[leafPos - 1] = leaf;

      --leafCount;

      for(int i = leafPos; i < leafCount; ++i)
        leaves[i] = leaves[i + 1];

      leaf->info      = expType::R;
      leaf->leafCount = 1;
      leaf->leaves    = new expNode*[1];
      leaf->leaves[0] = sLeaf;

      sLeaf->up = leaf;

      return (leafPos + 1);
    }

    // a [+] b
    int expNode::mergeBinary(const int leafPos){
      expNode *leaf   = leaves[leafPos];
      expNode *sLeafL = leaves[leafPos - 1];
      expNode *sLeafR = leaves[leafPos + 1];

      leaves[leafPos - 1] = leaf;

      leafCount -= 2;

      for(int i = leafPos; i < leafCount; ++i)
        leaves[i] = leaves[i + 2];

      leaf->info      = (expType::L | expType::R);
      leaf->leafCount = 2;
      leaf->leaves    = new expNode*[2];
      leaf->leaves[0] = sLeafL;
      leaf->leaves[1] = sLeafR;

      sLeafL->up = leaf;
      sLeafR->up = leaf;

      return leafPos;
    }

    // a [?] b : c
    int expNode::mergeTernary(const int leafPos){
      expNode *leaf   = leaves[leafPos];
      expNode *sLeafL = leaves[leafPos - 1];
      expNode *sLeafC = leaves[leafPos + 1];
      expNode *sLeafR = leaves[leafPos + 3];

      leaves[leafPos - 1] = leaf;

      leafCount -= 4;

      for(int i = leafPos; i < leafCount; ++i)
        leaves[i] = leaves[i + 4];

      leaf->info      = (expType::L | expType::C | expType::R);
      leaf->leafCount = 3;
      leaf->leaves    = new expNode*[3];
      leaf->leaves[0] = sLeafL;
      leaf->leaves[1] = sLeafC;
      leaf->leaves[2] = sLeafR;

      sLeafL->up = leaf;
      sLeafC->up = leaf;
      sLeafR->up = leaf;

      return leafPos;
    }


    //---[ Custom Functions ]---------
    void expNode::labelNewVariables(){

    }
    //================================


    //---[ Custom Type Info ]---------
    bool expNode::qualifierEndsWithStar() const {
      if( !(info & expType::qualifier) )
        return false;

      if(leafCount)
        return leaves[leafCount - 1]->qualifierEndsWithStar();
      else
        return (value == "*");
    }

    bool expNode::typeEndsWithStar() const {
      if( !(info & expType::type) ||
          (leafCount == 0) )
        return false;

      if(leaves[leafCount - 1]->info & expType::qualifier)
        return leaves[leafCount - 1]->qualifierEndsWithStar();

      return false;
    }

    bool expNode::hasAnArrayQualifier(const int pos) const {
      if( !(info & expType::qualifier) ||
          (leafCount <= pos) )
        return false;

      return ((leaves[pos]->value == "*") ||
              (leaves[pos]->value == "&") ||
              (leaves[pos]->value == "["));
    }
    //================================

    void expNode::swap(expNode &a, expNode &b){
      swapValues(a.sInfo, b.sInfo);

      swapValues(a.value, b.value);
      swapValues(a.info , b.info);

      swapValues(a.up, b.up);

      swapValues(a.leafCount, b.leafCount);
      swapValues(a.leaves   , b.leaves);

      swapValues(a.varType        , b.varType);
      swapValues(a.varPointerCount, b.varPointerCount);
    }

    expNode* expNode::clone(statement &s){
      expNode &newRoot = *(new expNode(s));

      newRoot.value = value;
      newRoot.info  = info;

      newRoot.leafCount = leafCount;

      newRoot.varType         = varType;
      newRoot.varPointerCount = varPointerCount;

      if(leafCount){
        newRoot.leaves = new expNode*[leafCount];

        for(int i = 0; i < leafCount; ++i){
          newRoot.leaves[i]     = clone(leaves[i]);
          newRoot.leaves[i]->up = &newRoot;
        }
      }

      return &newRoot;
    }

    expNode* expNode::clone(expNode *original){
      expNode &newLeaf = *(new expNode(*this));
      expNode &o = *original;

      newLeaf.value = o.value;
      newLeaf.info  = o.info;

      newLeaf.leafCount = o.leafCount;

      newLeaf.varType         = o.varType;
      newLeaf.varPointerCount = o.varPointerCount;

      if(o.leafCount){
        newLeaf.leaves = new expNode*[o.leafCount];

        for(int i = 0; i < o.leafCount; ++i){
          newLeaf.leaves[i]     = o.clone(o.leaves[i]);
          newLeaf.leaves[i]->up = &newLeaf;
        }
      }

      return &newLeaf;
    }

    void expNode::cloneTo(expNode &newRoot){
      newRoot.value = value;
      newRoot.info  = info;

      newRoot.leafCount = leafCount;

      newRoot.varType         = varType;
      newRoot.varPointerCount = varPointerCount;

      if(leafCount){
        newRoot.leaves = new expNode*[leafCount];

        for(int i = 0; i < leafCount; ++i){
          newRoot.leaves[i]     = clone(leaves[i]);
          newRoot.leaves[i]->up = &newRoot;
        }
      }
    }

    expNode* expNode::lastLeaf(){
      if(leafCount != 0)
        return leaves[leafCount - 1];

      return NULL;
    }

    //---[ Exp Info ]-----------------
    int expNode::depth(){
      int depth_ = 0;

      while(up)
        ++depth_;

      return depth_;
    }

    int expNode::whichLeafAmI(){
      if(up){
        const int upLeafCount = up->leafCount;

        for(int i = 0; i < upLeafCount; ++i)
          if(up->leaves[i] == this)
            return i;
      }

      return -1;
    }

    int expNode::nestedLeafCount(){
      int ret = leafCount;

      for(int i = 0; i < leafCount; ++i){
        if(leaves[i]->leafCount)
          ret += leaves[i]->nestedLeafCount();
      }

      return ret;
    }

    expNode* expNode::makeFlatHandle(){
      if(leafCount == 0)
        return NULL;

      expNode &flatNode = *(new expNode(*sInfo));

      flatNode.info   = expType::printLeaves;
      flatNode.leaves = new expNode*[nestedLeafCount()];

      int offset = 0;
      makeFlatHandle(offset, flatNode.leaves);

      flatNode.leafCount = offset;

      return &flatNode;
    }

    void expNode::makeFlatHandle(int &offset,
                                 expNode **flatLeaves){
      for(int i = 0; i < leafCount; ++i){
        switch(leaves[i]->info){
        case (expType::L):{
          flatLeaves[offset++] = leaves[i];
          flatLeaves[offset++] = leaves[i]->leaves[0];
          leaves[i]->leaves[0]->makeFlatHandle(offset, flatLeaves);

          break;
        }

        case (expType::R):{
          flatLeaves[offset++] = leaves[i]->leaves[0];
          leaves[i]->leaves[0]->makeFlatHandle(offset, flatLeaves);
          flatLeaves[offset++] = leaves[i];

          break;
        }

        case (expType::L | expType::R):{
          flatLeaves[offset++] = leaves[i]->leaves[0];
          leaves[i]->leaves[0]->makeFlatHandle(offset, flatLeaves);
          flatLeaves[offset++] = leaves[i];
          flatLeaves[offset++] = leaves[i]->leaves[1];
          leaves[i]->leaves[1]->makeFlatHandle(offset, flatLeaves);

          break;
        }

        case (expType::L | expType::C | expType::R):{
          flatLeaves[offset++] = leaves[i]->leaves[0];
          leaves[i]->leaves[0]->makeFlatHandle(offset, flatLeaves);
          flatLeaves[offset++] = leaves[i]->leaves[1];
          leaves[i]->leaves[1]->makeFlatHandle(offset, flatLeaves);
          flatLeaves[offset++] = leaves[i]->leaves[2];
          leaves[i]->leaves[2]->makeFlatHandle(offset, flatLeaves);

          break;
        }
        default:
          flatLeaves[offset++] = leaves[i];

          leaves[i]->makeFlatHandle(offset, flatLeaves);

          break;
        }
      }
    }


    void expNode::addNode(const int info_,
                          const int pos){
      expNode &newNode  = *(new expNode(*this));

      newNode.info      = info_;
      newNode.leafCount = 0;
      newNode.leaves    = NULL;

      expNode **newLeaves = new expNode*[leafCount + 1];

      //---[ Add Leaves ]-----
      for(int i = 0; i < pos; ++i)
        newLeaves[i] = leaves[i];

      newLeaves[pos] = &newNode;

      for(int i = pos; i < leafCount; ++i)
        newLeaves[i + 1] = leaves[i];
      //======================

      if(leafCount)
        delete [] leaves;

      leaves = newLeaves;
      ++leafCount;
    }

    void expNode::removeNode(const int pos){
      for(int i = (pos + 1); i < leafCount; ++i)
        leaves[i - 1] = leaves[i];

      --leafCount;
    }

    void expNode::convertTo(const int info_){
      if(info & expType::declaration){
        if(info_ & expType::variable){
          info = expType::variable;

          leafCount = 2;

          expNode *varNode = leaves[1]->leaves[0]->clone(*sInfo);

          leaves[1]->free();
          leaves[1] = varNode;
        }
      }
    }

    bool expNode::hasQualifier(const std::string &qualifier) const {
      if(info & expType::type){
        if(!leafCount ||
           !(leaves[0]->info & expType::qualifier))
          return false;

        return leaves[0]->hasQualifier(qualifier);
      }
      else if(info & expType::qualifier){
        if(leafCount){
          for(int i = 0; i < leafCount; ++i){
            if(leaves[i]->value == qualifier)
              return true;
          }

          return false;
        }
        else
          return value == qualifier;
      }
      else if(info & expType::variable){
        if((leafCount) &&
           (leaves[0]->info & expType::type)){

          return leaves[0]->hasQualifier(qualifier);
        }
        else
          return false;
      }

      return false;
    }

    void expNode::addQualifier(const std::string &qualifier,
                               const int pos){
      if(info & expType::variable){
        if(leafCount){
          expNode &lqNode = *(leaves[0]);

          if( !(lqNode.info & expType::type) ){
            std::cout << "5. Error on:" << *this << '\n';
            throw 1;
          }

          if( !(lqNode.leaves[0]->info & expType::qualifier) )
            lqNode.addNode(expType::qualifier, 0);

          expNode &qNode = *(lqNode.leaves[0]);

          qNode.addNode(expType::qualifier, pos);
          qNode.leaves[pos]->value = qualifier;
        }
      }
    }

    void expNode::addPostQualifier(const std::string &qualifier,
                                   const int pos){
      if(info & expType::variable){
        if(leafCount){
          expNode &lqNode = *(leaves[0]);

          if( !(lqNode.info & expType::type) ){
            std::cout << "5. Error on:" << *this << '\n';
            throw 1;
          }

          if( !(lqNode.lastLeaf()->info & expType::qualifier) )
            lqNode.addNode(expType::qualifier, lqNode.leafCount);

          expNode &qNode = *(lqNode.lastLeaf());

          qNode.addNode(expType::qualifier, pos);
          qNode.leaves[pos]->value = qualifier;
        }
      }
    }

    void expNode::removeQualifier(const std::string &qualifier){
      if(info & expType::type){
        if(leafCount){
          expNode &qNode = *(leaves[0]);

          if( !(qNode.info & expType::qualifier) )
            return;

          for(int i = 0; i < qNode.leafCount; ++i){
            if(qNode.leaves[i]->value == qualifier){
              qNode.removeNode(i);

              // Erase if there are no qualifiers
              if(qNode.leafCount == 0)
                removeNode(0);

              return;
            }
          }
        }
      }
    }

    void expNode::changeType(const std::string &newType){
      if(info & expType::variable){
        if(leafCount){
          const bool hasLQualifier = (leaves[0]->info & expType::type);

          if(hasLQualifier)
            leaves[0]->changeType(newType);
        }
      }
      else if(info & expType::type){
        if(leaves[0]->info & expType::type)
          leaves[0]->value = newType;
        else
          leaves[1]->value = newType;
      }
      else if(info & expType::declaration){
        if(leafCount &&
           (leaves[0]->info & expType::type)){

          leaves[0]->changeType(newType);
        }
      }
    }

    std::string expNode::getVariableName() const {
      if(info & expType::variable){
        if(leafCount){
          const bool hasLQualifier = (leaves[0]->info & expType::type);

          return leaves[hasLQualifier]->value;
        }
        else
          return value;
      }

      return "";
    }

    void expNode::setVarInfo(varInfo &var){
      if(info & expType::type){
        const bool hasLQualifier = ((leaves[0]->info & expType::qualifier) ||
                                    (leaves[0]->leafCount &&
                                     (leaves[0]->info & expType::type)));

        const bool hasRQualifier = (((hasLQualifier + 1) < leafCount) &&
                                    (leaves[hasLQualifier + 1]->info & (expType::qualifier |
                                                                        expType::type)));

        if(hasLQualifier){
          expNode &lq = *(leaves[0]);

          for(int i = 0; i < lq.leafCount; ++i)
            var.descriptors.push_back(lq.leaves[i]->value);
        }

        var.type = sInfo->hasTypeInScope(leaves[hasLQualifier]->value);

        if(hasRQualifier){
          expNode &rq = *(leaves[hasLQualifier + 1]);

          // [-] Missing * [const] * [const]
          for(int i = 0; i < rq.leafCount; ++i){
            if(rq.leaves[i]->value == "*")
              ++(var.pointerCount);
          }
        }
      }
      else if(info & expType::qualifier){
        if(leafCount){
          for(int i = 0; i < leafCount; ++i){
            if(leaves[i]->value == "*")
              ++(var.pointerCount);
            else if(leaves[i]->info & expType::C){
              std::string stackSize = (std::string) *(leaves[i]);

              // var[x]
              if(2 < stackSize.size())
                var.stackPointerSizes.push_back( stackSize.substr(1, stackSize.size() - 2) );
            }
          }
        }
        else
          var.descriptors.push_back(value);
      }

      if(var.pointerCount)
        var.typeInfo |= heapPointerType;
    }
    //================================

    void expNode::freeLeaf(const int leafPos){
      leaves[leafPos]->free();
      delete leaves[leafPos];
    }

    void expNode::free(){
      for(int i = 0; i < leafCount; ++i){
        leaves[i]->free();
        delete leaves[i];
      }

      leafCount = 0;
      delete [] leaves;
    }

    void expNode::print(const std::string &tab){
      std::cout << tab << "[" << getBits(info) << "] " << value << '\n';

      for(int i = 0; i < leafCount; ++i)
        leaves[i]->print(tab + "    ");
    }

    void expNode::printOn(std::ostream &out, const std::string &tab){
      switch(info){
      case (expType::root):{
        out << tab;

        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        // Change later
        if((up == NULL) &&
           (sInfo->type & declareStatementType))
          out << ';';

        break;
      }

      case (expType::L):{
        out << value << *(leaves[0]);

        break;
      }

      case (expType::R):{
        out << *(leaves[0]) << value;

        break;
      }

      case (expType::L | expType::R):{
        if((value != ".") && (value != "->"))
          out << *(leaves[0]) << ' ' << value << ' ' << *(leaves[1]);
        else
          out << *(leaves[0]) << value << *(leaves[1]);

        break;
      }

      case (expType::L | expType::C | expType::R):{
        out << *(leaves[0]) << '?' << *(leaves[1]) << ':' << *(leaves[2]);

        break;
      }

      case (expType::C):{
        const char startChar = value[0];

        out << startChar;

        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        out << (char) ((')' * (startChar == '(')) +
                       (']' * (startChar == '[')) +
                       ('}' * (startChar == '{')));

        break;
      }

      case (expType::qualifier):{
        if(leafCount){
          out << *(leaves[0]);

          for(int i = 1; i < leafCount; ++i){
            if( !hasAnArrayQualifier(i) )
              out << ' ';

            out << *(leaves[i]);
          }
        }
        else{
          out << value;
        }

        break;
      }

      case (expType::type):{
        // [const] [int] [*]
        if(leafCount){
          out << *(leaves[0]);

          for(int i = 1; i < leafCount; ++i){
            if( !leaves[i - 1]->hasAnArrayQualifier() )
              out << ' ';

            out << *(leaves[i]);
          }

          if(leaves[leafCount - 1]->info & expType::type)
            out << ' ';
        }
        // [int]
        else{
          out << value;
        }

        break;
      }

      case (expType::type | expType::namespace_):{
        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        break;
      }

      case (expType::presetValue):{
        out << value;

        break;
      }

      case (expType::operator_):{
        out << value;

        break;
      }

      case (expType::unknown):{
        out << value;

        break;
      }

      case (expType::variable):{
        // [[[const] [int] [*]] [x]]
        if(leafCount){
          const bool hasLQualifier = (leaves[0]->info & (expType::qualifier |
                                                         expType::type));

          const bool hasRQualifier = (((hasLQualifier + 1) < leafCount) &&
                                      (leaves[hasLQualifier + 1]->info & (expType::qualifier |
                                                                          expType::type)));

          if(hasLQualifier){
            out << *(leaves[0]);

            if((leaves[0]->info & expType::qualifier) ||
               ((leaves[0]->leafCount) &&
                (leaves[0]->lastLeaf()->info & expType::qualifier))){

              out << ' ';
            }
          }

          out << *(leaves[hasLQualifier]);

          if(hasRQualifier){
            if( !(leaves[hasLQualifier + 1]->hasAnArrayQualifier()) )
              out << ' ';

            out << *(leaves[hasLQualifier + 1]);
          }
        }
        // [x]
        else{
          out << value;
        }

        break;
      }

      case (expType::function | expType::prototype):{
        if(leafCount){
          for(int i = 0; i < (leafCount - 1); ++i)
            out << *(leaves[i]);

          expNode &argNode = *(leaves[leafCount - 1]);

          out << '(';

          for(int i = 0; i < (argNode.leafCount - 1); ++i)
            out << *(argNode.leaves[i]) << ", ";

          out << *(argNode.leaves[argNode.leafCount - 1]);

          out << ");\n";
        }

        break;
      }

      case (expType::function | expType::declaration):{
        if(leafCount){
          for(int i = 0; i < (leafCount - 1); ++i)
            out << *(leaves[i]);

          expNode &argNode = *(leaves[leafCount - 1]);

          out << '(';

          for(int i = 0; i < (argNode.leafCount - 1); ++i)
            out << *(argNode.leaves[i]) << ", ";

          out << *(argNode.leaves[argNode.leafCount - 1]);

          out << ')';
        }

        break;
      }

      case (expType::function):{
        out << value;

        if(leafCount)
          out << *(leaves[0]);

        break;
      }

      case (expType::functionPointer):{
        out << *(leaves[0]) << " (*" << *(leaves[1]) << ")"
            << '(';

        expNode *argNode = leaves[2];

        if(argNode->leafCount){
          for(int i = 0; i < (argNode->leafCount - 1); ++i)
            out << *(argNode->leaves[i]) << ", ";

          out << *(argNode->leaves[argNode->leafCount - 1]);
        }

        out << ')';

        break;
      }

      case (expType::declaration):{
        if(leafCount){
          // Delarations are used in
          //   typedef struct {} [a]
          // where there is no type
          const int typeAndVar = 2 - !(leaves[0]->info & expType::type);

          if(typeAndVar == 2)
            out << tab;

          for(int i = 0; i < typeAndVar; ++i)
            out << *(leaves[i]);

          for(int i = typeAndVar; i < leafCount; ++i)
            out << ", " << *(leaves[i]);

          if(typeAndVar == 2)
            out << ";\n";
        }

        break;
      }

      case (expType::struct_):{
        if(leafCount){
          out << tab;

          // Qualifiers
          for(int i = 0; i < leafCount; ++i){
            expNode &leaf = *(leaves[i]);

            if(leaf.value == "{"){
              out << "{\n";

              for(int j = 0; j < leaf.leafCount; ++j)
                leaf.leaves[j]->printOn(out, tab + "  ");

              if(i != (leafCount - 1))
                out << tab << "} ";
              else
                out << tab << "};\n";
            }
            else{
              out << leaf;

              if(i < (leafCount - 1))
                out << ' ';
            }
          }
        }
        else
          out << ";";

        break;
      }

      case (expType::C | expType::cast_):{
        out << '(';

        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        out << ')';

        break;
      }

      case (expType::namespace_):{
        break;
      }

      case (expType::macro_):{
        out << tab << value << '\n';
        break;
      }

      case (expType::goto_):{
        out << tab << "goto " << value << ';';
        break;
      }

      case (expType::gotoLabel_):{
        out << tab << value << ':';
        break;
      }

      case (expType::case_):{
        out << tab << "case ";

        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        if((up == NULL) &&
           (sInfo->type & simpleStatementType))
          out << ';';

        break;
      }

      case (expType::return_):{
        out << tab;

        for(int i = 0; i < leafCount; ++i)
          out << *(leaves[i]);

        out << '\n';

        break;
      }

      case (expType::occaFor):{
        out << tab << value;
        break;
      }

      case (expType::checkSInfo):{
        if(sInfo->type & flowStatementType){
          out << tab;

          if(sInfo->type & forStatementType)
            out << "for(";
          else if(sInfo->type & whileStatementType)
            out << "while(";
          else if(sInfo->type & doWhileStatementType)
            out << "do";
          else if(sInfo->type & ifStatementType)
            out << "if(";
          else if(sInfo->type & elseIfStatementType)
            out << "else if(";
          else if(sInfo->type & elseStatementType)
            out << "else";
          else if(sInfo->type & switchStatementType)
            out << "switch(";

          if(leafCount){
            for(int i = 0; i < (leafCount - 1); ++i)
              out << *(leaves[i]) << "; ";

            out << *(leaves[leafCount - 1]);
          }

          if( !(sInfo->type & (doWhileStatementType |
                                elseStatementType    |
                                gotoStatementType)) )
            out << ")";
          else if(sInfo->type & gotoStatementType)
            out << ":";
        }
      }

      case (expType::printValue):{
        out << value << ' ';

        break;
      }

      case (expType::printLeaves):{
        if(leafCount){
          for(int i = 0; i < leafCount; ++i)
            out << leaves[i]->value << ' ';

          out << '\n';
        }

        break;
      }

      default:{
        if(info & expType::typedef_){
          const int oldInfo = info;

          out << "typedef ";

          info &= ~expType::typedef_;

          out << *this;

          info = oldInfo;

          if(info & expType::struct_)
            out << ";\n";
        }
      }
      };
    }

    std::string expNode::getString(const std::string &tab){
      std::stringstream ss;

      printOn(ss, tab);

      return ss.str();
    }

    expNode::operator std::string (){
      return getString();
    }

    std::ostream& operator << (std::ostream &out, expNode &n){
      n.printOn(out);

      return out;
    }
    //==============================================


    //---[ Statement Functions ]--------------------
    statement::statement(parserBase &pb) :
      depth(-1),
      type(blockStatementType),

      up(NULL),

      varOriginMap(pb.varOriginMap),
      varUsedMap(pb.varUsedMap),

      expRoot(*this),

      statementCount(0),
      statementStart(NULL),
      statementEnd(NULL) {}

    statement::statement(const int depth_,
                         const int type_,
                         statement *up_) :
      depth(depth_),
      type(type_),

      up(up_),

      varOriginMap(up_->varOriginMap),
      varUsedMap(up_->varUsedMap),

      expRoot(*this),

      statementCount(0),
      statementStart(NULL),
      statementEnd(NULL) {}

    statement::~statement(){};

    statement* statement::makeSubStatement(){
      return new statement(depth + 1,
                           0, this);
    }

    std::string statement::getTab() const {
      std::string ret = "";

      for(int i = 0; i < depth; ++i)
        ret += "  ";

      return ret;
    }

    int statement::statementType(strNode *&nodeRoot){
      if(nodeRoot == NULL)
        return invalidStatementType;

      else if(nodeRoot->type == macroKeywordType)
        return checkMacroStatementType(nodeRoot);

      else if(nodeRoot->type == keywordType["occaOuterFor0"])
        return checkOccaForStatementType(nodeRoot);

      else if(nodeRoot->type & structType)
        return checkStructStatementType(nodeRoot);

      else if(nodeRoot->type & operatorType)
        return checkUpdateStatementType(nodeRoot);

      else if(nodeHasDescriptor(nodeRoot))
        return checkDescriptorStatementType(nodeRoot);

      else if(nodeRoot->type & unknownVariable){
        if(nodeRoot->right &&
           nodeRoot->right->value == ":")
          return checkGotoStatementType(nodeRoot);

        return checkUpdateStatementType(nodeRoot);
      }

      else if(nodeRoot->type & flowControlType)
        return checkFlowStatementType(nodeRoot);

      else if(nodeRoot->type & specialKeywordType)
        return checkSpecialStatementType(nodeRoot);

      else if((nodeRoot->type == startBrace) &&
              (nodeRoot->up)                 &&
              !(nodeRoot->up->type & operatorType))
        return checkBlockStatementType(nodeRoot);

      while(nodeRoot &&
            !(nodeRoot->type & endStatement))
        nodeRoot = nodeRoot->right;

      return declareStatementType;
    }

    int statement::checkMacroStatementType(strNode *&nodeRoot){
      return macroStatementType;
    }

    int statement::checkOccaForStatementType(strNode *&nodeRoot){
      return keywordType["occaOuterFor0"];
    }

    int statement::checkStructStatementType(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & structType)
          break;

        nodeRoot = nodeRoot->right;
      }

      if((nodeRoot->value == "struct")       &&
         (nodeRoot->down.size() == 0)        &&
         (nodeRoot->right)                   &&
         (hasTypeInScope(nodeRoot->right->value))){

          return checkDescriptorStatementType(nodeRoot);
      }

      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return structStatementType;
    }

    int statement::checkUpdateStatementType(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return updateStatementType;
    }

    int statement::checkDescriptorStatementType(strNode *&nodeRoot){
#if 1
      strNode *oldNodeRoot = nodeRoot;
      strNode *nodePos;

      // Skip descriptors
      while((nodeRoot)                         &&
            (nodeRoot->down.size() == 0)       &&
            ((nodeRoot->type & descriptorType) ||
             nodeHasSpecifier(nodeRoot))){

        nodeRoot = nodeRoot->right;
      }

      nodePos = nodeRoot;

      while((nodeRoot) &&
            !(nodeRoot->type & endStatement))
        nodeRoot = nodeRoot->right;

      const int downCount = nodePos->down.size();

      // Function {Proto, Def | Ptr}
      if(downCount && (nodePos->down[0]->type & parentheses)){
        if(downCount == 1)
          return functionPrototypeType;

        strNode *downNode = nodePos->down[1];

        if(downNode->type & brace){
          nodeRoot = nodePos;
          return functionDefinitionType;
        }

        else if(downNode->type & parentheses){
          downNode = downNode->right;

          if(downNode->type & parentheses){
            std::cout << "Function pointer needs a [*]:\n"
                      << prettyString(oldNodeRoot, "  ");
            throw 1;
          }

          while(downNode->value == "*"){
            if(downNode->down.size()){
              if(nodeRoot == NULL){
                std::cout << "Missing a [;] after:\n"
                          << prettyString(oldNodeRoot, "  ");
                throw 1;
              }

              return declareStatementType;
            }

            downNode = downNode->right;
          }

          if(nodeRoot == NULL){
            std::cout << "Missing a [;] after:\n"
                      << prettyString(oldNodeRoot, "  ");
            throw 1;
          }

          // [C++] Function call, not function pointer define
          //    getFunc(0)(arg1, arg2);
          if(hasVariableInScope(downNode->value))
            return updateStatementType;

          return declareStatementType;
        }
        else{
          std::cout << "You found the [Waldo 1] error in:\n"
                    << prettyString(oldNodeRoot, "  ");
          throw 1;
        }
      }

      if(nodeRoot == NULL){
        std::cout << "Missing a [;] after:\n"
                  << prettyString(oldNodeRoot, "  ");
        throw 1;
      }

      return declareStatementType;
#else
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          return declareStatementType;

        // Case:
        //   const varName = ();
        if(nodeRoot->type & operatorType){
          while(nodeRoot){
            if(nodeRoot->type & endStatement)
              break;

            nodeRoot = nodeRoot->right;
          }

          return declareStatementType;
        }

        else if(nodeRoot->down.size() &&
                (nodeRoot->down[0]->type & parentheses)){

          const int downCount = nodeRoot->down.size();

          if(downCount == 1){
            while(nodeRoot){
              if(nodeRoot->type & endStatement)
                break;

              if(nodeRoot->right == NULL){
                std::cout << "Missing a [;] after:\n"
                          << prettyString(nodeRoot, "  ");
                throw 1;
              }

              nodeRoot = nodeRoot->right;
            }

            return functionPrototypeType;
          }
          else
            return functionDefinitionType;
        }

        if(nodeRoot->right == NULL)
          break;

        nodeRoot = nodeRoot->right;
      }

      return declareStatementType;
#endif
    }

    int statement::checkGotoStatementType(strNode *&nodeRoot){
      nodeRoot = nodeRoot->right;
      return gotoStatementType;
    }

    int statement::checkFlowStatementType(strNode *&nodeRoot){
      if(nodeRoot->value == "for")
        return forStatementType;
      else if(nodeRoot->value == "while")
        return whileStatementType;
      else if(nodeRoot->value == "do")
        return doWhileStatementType;
      else if(nodeRoot->value == "if")
        return ifStatementType;
      else if(nodeRoot->value == "else if")
        return elseIfStatementType;
      else if(nodeRoot->value == "else")
        return elseStatementType;
      else if(nodeRoot->value == "switch")
        return switchStatementType;

      std::cout << "You found the [Waldo 2] error in:\n"
                << prettyString(nodeRoot, "  ");
      throw 1;

      return 0;
    }

    int statement::checkSpecialStatementType(strNode *&nodeRoot){
      while(nodeRoot){
        if(nodeRoot->type & endStatement)
          break;

        nodeRoot = nodeRoot->right;
      }

      return blankStatementType;
    }

    int statement::checkBlockStatementType(strNode *&nodeRoot){
      nodeRoot = lastNode(nodeRoot);

      return blockStatementType;
    }

    void statement::addTypeDef(const std::string &typeDefName){
      typeDef &def = *(new typeDef);
      def.typeName = typeDefName;
      scopeTypeMap[typeDefName] = &def;
    }

    bool statement::nodeHasQualifier(strNode *n) const {
      if( !(n->type & qualifierType) )
        return false;

      // short and long can be both:
      //    specifiers and qualifiers
      if( !(n->type & specifierType) )
        return true;

      if((n->right) == NULL)
        return false;

      return (n->right->type & descriptorType);
    }

    bool statement::nodeHasSpecifier(strNode *n) const {
      return ((n->type & specifierType) ||
              ((n->type & unknownVariable) &&
               ( hasTypeInScope(n->value) )));
    }

    bool statement::nodeHasDescriptor(strNode *n) const {
      if(nodeHasSpecifier(n) || nodeHasQualifier(n))
        return true;

      return false;
    }

    varInfo statement::loadVarInfo(strNode *&nodePos){
      varInfo info;

#if 0
      while(nodePos &&
            ((nodePos->type & qualifierType) ||
             nodeHasSpecifier(nodePos))){

        if(nodePos->type & structType){
          info.descriptors.push_back(*nodePos);
          info.typeInfo |= structType;
        }

        else if(nodeHasQualifier(nodePos)){
          if(nodePos->value == "*"){
            info.typeInfo |= heapPointerType;
            ++info.pointerCount;

            if(nodePos->right &&
               nodePos->right->value == "const"){
              info.typeInfo |= constPointerType;
              nodePos = nodePos->right;
            }
          }
          else if(nodePos->value == "&")
            info.typeInfo |= referenceType;
          else{
            if((nodePos->value  == "texture")              ||
               ((nodePos->value == "image1d_t")            ||
                (nodePos->value == "image2d_t"))           ||
               (nodePos->value  == "cudaSurfaceObject_t"))
              info.typeInfo |= textureType;

            info.descriptors.push_back(nodePos->value);
          }
        }

        else if(nodeHasSpecifier(nodePos)){
          info.type = hasTypeInScope(*nodePos);

          if(info.type == NULL){
            std::cout << "Type [" << *nodePos << "] is not defined.\nFound in:\n";
            nodePos->print();
            throw 1;
          }
        }

        if(nodePos->down.size() != 0)
          break;

        nodePos = nodePos->right;
      }

      if((nodePos == NULL) ||
         (nodePos->type & (endSection |
                           endStatement))){

        if(nodePos &&
           nodePos->value == ":"){

          info.bitField = atoi(nodePos->right->value.c_str());
          nodePos = nodePos->right->right;
        }

        return info;
      }

      const int downCount = nodePos->down.size();

      if(downCount == 0){
        info.name      = nodePos->value;
        info.typeInfo |= variableType;

        nodePos = nodePos->right;

        if(nodePos &&
           nodePos->value == ":"){

          info.bitField = atoi(nodePos->right->value.c_str());
          nodePos = nodePos->right->right;

          return info;
        }
      }
      else{
        strNode *downNode = nodePos->down[0];

        if(downNode->type == startParentheses){
          // [-] Only for C
          if((2 <= downCount) &&
             (nodePos->down[1]->type == startParentheses)){

            downNode = downNode->right;

            while(downNode->value == "*"){
              ++(info.pointerCount);
              downNode = downNode->right;
            }

            info.name      = downNode->value;
            info.typeInfo |= functionPointerType;

            if(downNode->down.size()){
              const int downCount2 = downNode->down.size();

              for(int i = 0; i < downCount2; ++i){
                if(downNode->down[i]->type != startBracket)
                  break;

                std::string sps = prettyString(downNode->down[i]);
                sps = sps.substr(1, sps.size() - 2); // Remove '[' and ']'

                info.stackPointerSizes.push_back(sps);
              }
            }

            downNode = nodePos->down[1]->right;

            while(downNode){
              varInfo &arg = *(new varInfo);
              arg = loadVarInfo(downNode);

              info.vars.push_back(&arg);

              // Loaded last arg
              if((downNode == NULL) ||
                 (downNode->right == NULL))
                break;

              downNode = downNode->right;
            }

            nodePos = nodePos->right;
          }
          else{
            info.name      = nodePos->value;
            info.typeInfo |= functionType;

            downNode = downNode->right;

            while(downNode){
              varInfo &arg = *(new varInfo);
              arg = loadVarInfo(downNode);

              info.vars.push_back(&arg);

              downNode = downNode->right;

              // Loaded last arg
              if(downNode == NULL)
                break;
            }

            // Distinguish between prototypes and function calls
            if(nodePos->right &&
               (nodePos->right->value == ";")){
              if(info.type)
                info.typeInfo |= protoType;
              else
                info.typeInfo |= functionCallType;
            }
          }
        }
        else if(downNode->type == startBracket){
          info.name      = nodePos->value;
          info.typeInfo |= (variableType | stackPointerType);

          for(int i = 0; i < downCount; ++i){
            if(nodePos->down[i]->type != startBracket)
              break;

            std::string sps = prettyString(nodePos->down[i]);
            sps = sps.substr(1, sps.size() - 2); // Remove '[' and ']'

            info.stackPointerSizes.push_back(sps);
          }
        }
      }

      return info;
#else
      while(nodePos                         &&
            !(nodePos->type & presetValue)  &&
            !(nodePos->type & endStatement) && // For bitfields
            (!(nodePos->type & unknownVariable) ||
             hasTypeInScope(nodePos->value))){

        if(nodePos->type & structType){
          info.descriptors.push_back(*nodePos);
          info.typeInfo |= structType;
        }

        else if(nodeHasQualifier(nodePos)){
          if(nodePos->value == "*"){
            info.typeInfo |= heapPointerType;
            ++info.pointerCount;

            if(nodePos->right &&
               nodePos->right->value == "const"){
              info.typeInfo |= constPointerType;
              nodePos = nodePos->right;
            }
          }
          else if(nodePos->value == "&")
            info.typeInfo |= referenceType;
          else{
            if((nodePos->value  == "texture")              ||
               ((nodePos->value == "image1d_t")            ||
                (nodePos->value == "image2d_t"))           ||
               (nodePos->value  == "cudaSurfaceObject_t"))
              info.typeInfo |= textureType;

            info.descriptors.push_back(*nodePos);
          }
        }

        else if(nodeHasSpecifier(nodePos)){
          info.type = hasTypeInScope(*nodePos);

          if(info.type == NULL){
            std::cout << "Type [" << *nodePos << "] is not defined.\nFound in:\n";
            nodePos->print();
            throw 1;
          }
        }

        nodePos = nodePos->right;
      }

      if((nodePos == NULL) ||
         (nodePos->type & endStatement)) // For bitfields
        return info;

      info.name = *nodePos;

      const int downCount = nodePos->down.size();

      if(downCount){
        if(nodePos->down[0]->type == startParentheses){
          strNode *argPos = nodePos->down[0];
          info.typeInfo |= functionType;

          strNode *lastPos = lastNode(nodePos);

          // Distinguish between prototypes and function calls
          if(lastPos->value == ";"){
            if(info.type)
              info.typeInfo |= protoType;
            else
              info.typeInfo |= functionCallType;
          }
        }
        else if(nodePos->down[0]->type == startBracket){
          info.typeInfo |= (variableType | stackPointerType);

          for(int i = 0; i < downCount; ++i){
            nodePos->down[i]->type = startBracket;

            std::string sps = prettyString(nodePos->down[i]);
            sps = sps.substr(1, sps.size() - 2); // Remove '[' and ']'

            info.stackPointerSizes.push_back(sps);
          }
        }
      }
      else{
        info.typeInfo |= variableType;
        nodePos = nodePos->right;
      }

      return info;
#endif
    }

    typeDef* statement::hasTypeInScope(const std::string &typeName) const {
      cScopeTypeMapIterator it = scopeTypeMap.find(typeName);

      if(it != scopeTypeMap.end())
        return it->second;

      if(up)
        return up->hasTypeInScope(typeName);

      return NULL;
    }

    varInfo* statement::hasVariableInScope(const std::string &varName) const {
      const statement *sPos = this;

      while(sPos){
        cScopeVarMapIterator it = sPos->scopeVarMap.find(varName);

        if(it != sPos->scopeVarMap.end())
          return it->second;

        sPos = sPos->up;
      }

      return NULL;
    }

    bool statement::hasDescriptorVariable(const std::string descriptor) const {
      cScopeVarMapIterator it = scopeVarMap.begin();

      while(it != scopeVarMap.end()){
        if((it->second)->hasDescriptor(descriptor))
          return true;

        ++it;
      }

      return false;
    }

    bool statement::hasDescriptorVariableInScope(const std::string descriptor) const {
      if(hasDescriptorVariable(descriptor))
        return true;

      if(up != NULL)
        return up->hasDescriptorVariable(descriptor);

      return false;
    }

    void statement::loadAllFromNode(strNode *nodeRoot){
      while(nodeRoot)
        nodeRoot = loadFromNode(nodeRoot);
    }

    strNode* statement::loadFromNode(strNode *nodeRoot){
      if(nodeRoot->type == 0){
        const int downCount = nodeRoot->down.size();

        for(int i = 0; i < downCount; ++i)
          loadFromNode(nodeRoot->down[i]);

        return nodeRoot->right;
      }

      statement *newStatement = makeSubStatement();
      strNode * nodeRootEnd   = nodeRoot;

      newStatement->expRoot.loadFromNode(nodeRootEnd);
      const int st = newStatement->type;

      if(st & invalidStatementType){
        std::cout << "Not a valid statement\n";
        throw 1;
      }

      if( !(st & ifStatementType) )
        addStatement(newStatement);

      if(st & simpleStatementType){
        nodeRootEnd = newStatement->loadSimpleFromNode(st,
                                                       nodeRoot,
                                                       nodeRootEnd);
      }

      else if(st & flowStatementType){
        if(st & forStatementType)
          nodeRootEnd = newStatement->loadForFromNode(st,
                                                      nodeRoot,
                                                      nodeRootEnd);

        else if(st & whileStatementType)
          nodeRootEnd = newStatement->loadWhileFromNode(st,
                                                        nodeRoot,
                                                        nodeRootEnd);

        else if(st & ifStatementType){
          delete newStatement;

          nodeRootEnd = loadIfFromNode(st,
                                       nodeRoot,
                                       nodeRootEnd);
        }

        else if(st & switchStatementType)
          nodeRootEnd = newStatement->loadSwitchFromNode(st,
                                                         nodeRoot,
                                                         nodeRootEnd);

        else if(st & gotoStatementType)
          nodeRootEnd = newStatement->loadGotoFromNode(st,
                                                       nodeRoot,
                                                       nodeRootEnd);
      }
      else if(st & functionStatementType){
        if(st & functionDefinitionType)
          nodeRootEnd = newStatement->loadFunctionDefinitionFromNode(st,
                                                                     nodeRoot,
                                                                     nodeRootEnd);

        else if(st & functionPrototypeType)
          nodeRootEnd = newStatement->loadFunctionPrototypeFromNode(st,
                                                                    nodeRoot,
                                                                    nodeRootEnd);
      }
      else if(st & blockStatementType)
        nodeRootEnd = newStatement->loadBlockFromNode(st,
                                                      nodeRoot,
                                                      nodeRootEnd);

      else if(st & structStatementType)
        nodeRootEnd = newStatement->loadStructFromNode(st,
                                                       nodeRoot,
                                                       nodeRootEnd);

      else if(st & blankStatementType)
        nodeRootEnd = newStatement->loadBlankFromNode(st,
                                                      nodeRoot,
                                                      nodeRootEnd);

      else if(st & macroStatementType)
        nodeRootEnd = newStatement->loadMacroFromNode(st,
                                                      nodeRoot,
                                                      nodeRootEnd);

      return nodeRootEnd;
    }

    expNode* statement::createExpNodeFrom(const std::string &source){
      strNode *nodeRoot = parserNamespace::splitContent(source);
      nodeRoot          = parserNamespace::labelCode(nodeRoot);

      loadFromNode(nodeRoot);

      statementNode *sn = statementEnd;

      statementEnd = statementEnd->left;

      if(statementEnd)
        statementEnd->right = NULL;

      --(statementCount);

      expNode &ret = *(sn->value->expRoot.clone(*this));

      delete sn->value;
      delete sn;

      return &ret;
    }

    void statement::loadBlocksFromLastNode(strNode *end,
                                           const int startBlockPos){
      if(end == NULL)
        return;

      const int downCount = end->down.size();

      if(startBlockPos <= downCount){
        for(int i = startBlockPos; i < downCount; ++i)
          up->loadAllFromNode(end->down[i]);

        end->down.erase(end->down.begin() + startBlockPos,
                        end->down.end());
      }
    }

    strNode* statement::loadSimpleFromNode(const int st,
                                           strNode *nodeRoot,
                                           strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    strNode* statement::loadForFromNode(const int st,
                                        strNode *nodeRoot,
                                        strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      const int downCount = nodeRootEnd->down.size();

      if(downCount == 1){
        // for(;;)
        //   stuff;
        if(nodeRoot->down[0]->type == startParentheses)
          return loadFromNode(nextNode);

        // occaOuterFor {
        // }
        else{
          strNode *blockStart = nodeRootEnd->down[0];
          strNode *blockEnd   = lastNode(blockStart);

          nodeRootEnd->down.erase(nodeRootEnd->down.begin() + 0,
                                  nodeRootEnd->down.begin() + 1);

          // Load all down's before popping [{] and [}]'s
          const int blockDownCount = blockStart->down.size();

          for(int i = 0; i < blockDownCount; ++i)
            loadAllFromNode( blockStart->down[i] );

          if(blockStart->right != blockEnd){
            popAndGoRight(blockStart);
            popAndGoLeft(blockEnd);

            loadAllFromNode(blockStart);
          }
        }
      }
      else{
        strNode *blockStart = nodeRootEnd->down[1];
        strNode *blockEnd   = lastNode(blockStart);

        nodeRootEnd->down.erase(nodeRootEnd->down.begin() + 1,
                                nodeRootEnd->down.begin() + 2);

        // Load all down's before popping [{] and [}]'s
        const int blockDownCount = blockStart->down.size();

        for(int i = 0; i < blockDownCount; ++i)
          loadAllFromNode( blockStart->down[i] );

        loadBlocksFromLastNode(nodeRootEnd, 1);

        popAndGoRight(blockStart);
        popAndGoLeft(blockEnd);

        loadAllFromNode(blockStart);
      }

      return nextNode;
    }

    strNode* statement::loadWhileFromNode(const int st,
                                          strNode *nodeRoot,
                                          strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      int allowedDowns = (st == whileStatementType);

      const int downCount = nodeRootEnd->down.size();

      if(downCount == allowedDowns){
        return loadFromNode(nextNode);
      }

      else{
        strNode *blockStart = nodeRoot->down[allowedDowns];
        strNode *blockEnd   = lastNode(blockStart);

        nodeRoot->down.erase(nodeRoot->down.begin() + allowedDowns + 0,
                             nodeRoot->down.begin() + allowedDowns + 1);

        // Load all down's before popping [{] and [}]'s
        const int downCount = blockStart->down.size();

        for(int i = 0; i < downCount; ++i)
          loadAllFromNode( blockStart->down[i] );

        loadBlocksFromLastNode(nodeRootEnd, allowedDowns);

        popAndGoRight(blockStart);
        popAndGoLeft(blockEnd);

        loadAllFromNode(blockStart);
      }

      return nextNode;
    }

    strNode* statement::loadIfFromNode(const int st_,
                                       strNode *nodeRoot,
                                       strNode *nodeRootEnd){
      strNode *nextNode;
      int st = st_;

      do {
        statement *newStatement = makeSubStatement();
        strNode * nodeRootEnd = nodeRoot;

        newStatement->expRoot.loadFromNode(nodeRootEnd);

        st = newStatement->type;

        if(st & invalidStatementType){
          std::cout << "Not a valid statement\n";
          throw 1;
        }

        nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

        if(nodeRoot)
          nodeRoot->left = NULL;
        if(nodeRootEnd)
          nodeRootEnd->right = NULL;

        const int downCount = nodeRootEnd->down.size();

        if(((downCount == 1) && (st != elseStatementType)) ||
           ((downCount == 0) && (st == elseStatementType))){
          // if()           or    else
          //   statement;           statement;

          nextNode = newStatement->loadFromNode(nextNode);
          addStatement(newStatement);

          if(st == elseStatementType)
            break;
        }
        else{
          int blockPos = (st != elseStatementType) ? 1 : 0;

          strNode *blockStart = nodeRoot->down[blockPos];
          strNode *blockEnd   = lastNode(blockStart);

          nodeRoot->down.erase(nodeRoot->down.begin() + blockPos,
                               nodeRoot->down.begin() + blockPos + 1);

          // Load all down's before popping [{] and [}]'s
          const int blockDownCount = blockStart->down.size();

          for(int i = 0; i < blockDownCount; ++i)
            newStatement->loadAllFromNode( blockStart->down[i] );

          loadBlocksFromLastNode(nodeRootEnd, blockPos);

          popAndGoRight(blockStart);
          popAndGoLeft(blockEnd);

          newStatement->loadAllFromNode(blockStart);
          addStatement(newStatement);

          break;
        }

        if(nextNode == NULL)
          break;

        nodeRoot = nodeRootEnd = nextNode;

        // statement *newStatement = makeSubStatement();
        // strNode * nodeRootEnd = nodeRoot;

        // newStatement->expRoot.loadFromNode(nodeRootEnd);
        // const int st = newStatement->type;

        st = statementType(nodeRootEnd);

        if(st & invalidStatementType){
          std::cout << "Not a valid statement:\n";
          prettyString(nodeRoot, "", false);
          throw 1;
        }

      } while((st == elseIfStatementType) ||
              (st == elseStatementType));

      return nextNode;
    }

    // [-] Missing
    strNode* statement::loadSwitchFromNode(const int st,
                                           strNode *nodeRoot,
                                           strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    strNode* statement::loadGotoFromNode(const int st,
                                         strNode *nodeRoot,
                                         strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    strNode* statement::loadFunctionDefinitionFromNode(const int st,
                                                       strNode *nodeRoot,
                                                       strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      strNode *blockStart = nodeRootEnd->down[1];
      strNode *blockEnd   = lastNode(blockStart);

      nodeRootEnd->down.erase(nodeRootEnd->down.begin() + 1,
                              nodeRootEnd->down.begin() + 2);

      // Load all down's before popping [{] and [}]'s
      const int downCount = blockStart->down.size();

      bool brokeDowns = false;

      for(int i = 0; i < downCount; ++i){
        if(blockStart->down[i]->type != startBrace){
          if(i != 0)
            blockStart->down.erase(nodeRootEnd->down.begin(),
                                   nodeRootEnd->down.begin() + i);

          brokeDowns = true;
          break;
        }

        loadAllFromNode( blockStart->down[i] );
      }

      if(!brokeDowns){
        popAndGoRight(blockStart);
        popAndGoLeft(blockEnd);
      }
      else{
        blockStart->type  = 0;
        blockStart->value = "";

        popAndGoLeft(blockEnd);
      }

      if(brokeDowns &&
         (blockStart->right == blockEnd))
        return nextNode;

      loadAllFromNode(blockStart);

      return nextNode;
    }

    strNode* statement::loadFunctionPrototypeFromNode(const int st,
                                                      strNode *nodeRoot,
                                                      strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      return nextNode;
    }

    strNode* statement::loadBlockFromNode(const int st,
                                          strNode *nodeRoot,
                                          strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      // Load all down's before popping [{] and [}]'s
      const int downCount = nodeRoot->down.size();

      for(int i = 0; i < downCount; ++i)
        loadAllFromNode( nodeRoot->down[i] );

      if(nodeRoot->right == nodeRootEnd){
        popAndGoRight(nodeRoot);
        popAndGoLeft(nodeRootEnd);

        return nextNode;
      }

      popAndGoRight(nodeRoot);
      popAndGoLeft(nodeRootEnd);

      loadAllFromNode(nodeRoot);

      return nextNode;
    }

    strNode* statement::loadStructFromNode(const int st,
                                           strNode *nodeRoot,
                                           strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    // [-] Missing
    strNode* statement::loadBlankFromNode(const int st,
                                          strNode *nodeRoot,
                                          strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    // [-] Missing
    strNode* statement::loadMacroFromNode(const int st,
                                          strNode *nodeRoot,
                                          strNode *nodeRootEnd){
      strNode *nextNode = nodeRootEnd ? nodeRootEnd->right : NULL;

      if(nodeRoot)
        nodeRoot->left = NULL;
      if(nodeRootEnd)
        nodeRootEnd->right = NULL;

      loadBlocksFromLastNode(nodeRootEnd);

      return nextNode;
    }

    statementNode* statement::getStatementNode(){
      if(up != NULL){
        statementNode *ret = up->statementStart;
        const int upNodes  = up->statementCount;

        for(int i = 0; i < upNodes; ++i){
          if(ret->value == this)
            return ret;

          ret = ret->right;
        }
      }

      return NULL;
    }

    varInfo* statement::addVariable(const varInfo &info,
                                    statement *origin){
      scopeVarMapIterator it = scopeVarMap.find(info.name);

      if(it != scopeVarMap.end()       &&
         !info.hasDescriptor("extern") &&
         !((info.typeInfo & functionType) && ((it->second)->typeInfo & protoType))){

        std::cout << "Variable [" << info.name << "] defined in:\n"
                  << *origin
                  << "is already defined in:\n"
                  << *this;
        throw 1;
      }

      varInfo *&newInfo = scopeVarMap[info.name];

      newInfo = new varInfo(info);

      if(origin == NULL)
        varOriginMap[newInfo] = this;
      else{
        varOriginMap[newInfo]          = origin;
        origin->scopeVarMap[info.name] = newInfo;
      }

      return newInfo;
    }

    void statement::addStatement(statement *newStatement){
      if(statementStart != NULL){
        ++statementCount;
        statementEnd = statementEnd->push(newStatement);
      }
      else{
        statementCount = 1;
        statementStart = new node<statement*>(newStatement);
        statementEnd   = statementStart;
      }
    }

    statement* statement::clone(){
      statement *newStatement = new statement(depth,
                                              type, up);

      expRoot.cloneTo(newStatement->expRoot);

      newStatement->scopeVarMap = scopeVarMap;

      newStatement->statementCount = statementCount;

      newStatement->statementStart = NULL;
      newStatement->statementEnd   = NULL;

      if(statementCount == 0)
        return newStatement;

      statementNode *nodePos = statementStart;

      // [-] Broken
      // for(int i = 0; i < statementCount; ++i){
      //   newStatement->addStatement( nodePos->value->clone() );
      //   nodePos = nodePos->right;
      // }

      while(nodePos){
        newStatement->addStatement( nodePos->value->clone() );
        nodePos = nodePos->right;
      }

      return newStatement;
    }

    void statement::printVariablesInStatement(){
      scopeVarMapIterator it = scopeVarMap.begin();

      while(it != scopeVarMap.end()){
        std::cout << "  " << *(it->second) << '\n';

        ++it;
      }
    }

    void statement::printVariablesInScope(){
      if(up)
        up->printVariablesInScope();

      printVariablesInStatement();
    }

    void statement::printTypesInScope(){
      if(up)
        up->printTypesInScope();

      printTypesInStatement();
    }

    void statement::printTypesInStatement(){
      scopeTypeMapIterator it = scopeTypeMap.begin();

      while(it != scopeTypeMap.end()){
        std::cout << (it->first) << '\n';

        ++it;
      }
    }

    void statement::printTypeDefsInStatement(){
      scopeTypeMapIterator it = scopeTypeMap.begin();

      while(it != scopeTypeMap.end()){
        std::cout << (it->second)->print("  ") << '\n';

        ++it;
      }
    }

    //---[ Statement Info ]-----------
    void statement::swapExpWith(statement &s){
      swapValues(expRoot.value, s.expRoot.value);
      swapValues(expRoot.info , s.expRoot.info);

      swapValues(expRoot.leafCount, s.expRoot.leafCount);
      swapValues(expRoot.leaves   , s.expRoot.leaves);

      swapValues(expRoot.varType        , s.expRoot.varType);
      swapValues(expRoot.varPointerCount, s.expRoot.varPointerCount);
    }

    bool statement::hasQualifier(const std::string &qualifier) const {
      if(type & declareStatementType){
        return expRoot.leaves[0]->hasQualifier(qualifier);
      }
      else if(type & functionStatementType){
        expNode &typeNode = *(expRoot.leaves[0]);
        expNode &qualNode = *(typeNode.leaves[0]);

        if( !(qualNode.info & expType::qualifier) )
          return false;

        return qualNode.hasQualifier(qualifier);
      }
      else if(type & forStatementType){
        if(expRoot.leafCount){
          expNode &node1 = *(expRoot.leaves[0]);

          if((node1.leafCount) &&
             (node1.leaves[0]->info & expType::type)){

            return node1.leaves[0]->hasQualifier(qualifier);
          }
        }

        return false;
      }

      return false;
    }

    void statement::addQualifier(const std::string &qualifier,
                                 const int pos){
      if(hasQualifier(qualifier))
        return;

      if(type & declareStatementType){
      }
      else if(type & functionStatementType){
        expNode &typeNode = *(expRoot.leaves[0]);

        if( !(typeNode.leaves[0]->info & expType::qualifier) )
          typeNode.addNode(expType::qualifier, 0);

        expNode &qualNode = *(typeNode.leaves[0]);

        qualNode.addNode(expType::qualifier, pos);

        expNode &sNewQualNode = *(qualNode.leaves[pos]);
        sNewQualNode.value    = qualifier;
      }
      else if(type & forStatementType){
        if(expRoot.leafCount){
          expNode &node1    = *(expRoot.leaves[0]);
          expNode &qualNode = *(node1.leaves[0]);

          if( !(qualNode.leaves[0]->info & expType::qualifier) )
            qualNode.addNode(expType::qualifier, 0);

          qualNode.leaves[0]->value = qualifier;
        }
      }
    }

    void statement::removeQualifier(const std::string &qualifier){
      if(!hasQualifier(qualifier))
        return;

      if(type & declareStatementType){
        expRoot.leaves[0]->removeQualifier(qualifier);
      }
      else if(type & functionStatementType){
      }
      else if(type & forStatementType){
      }
    }

    expNode* statement::getDeclarationTypeNode(){
      if(type & declareStatementType)
        return expRoot.leaves[0];

      return NULL;
    }

    expNode* statement::getDeclarationVarNode(const int pos){
      if(type & declareStatementType)
        return expRoot.leaves[1 + pos];

      return NULL;
    }

    std::string statement::getDeclarationVarName(const int pos) const {
      if(type & declareStatementType){
        expNode &argNode = *(expRoot.leaves[1 + pos]);

        // First entry might be
        //   int [*] blah
        if(argNode.leaves[0]->info & expType::variable)
          return argNode.leaves[0]->getVariableName();
        else if(argNode.leaves[1]->info & expType::variable)
          return argNode.leaves[1]->getVariableName();
        else {
          // int i = 0  -->  [=] has [i,0]
          if(argNode.leaves[0]->info & expType::LCR){
            return argNode.leaves[0]->leaves[0]->getVariableName();
          }
          else {
            return argNode.leaves[1]->leaves[0]->getVariableName();
          }
        }
      }

      return "";
    }

    int statement::getDeclarationVarCount() const {
      if(type & declareStatementType)
        return (expRoot.leafCount - 1);

      return 0;
    }

    std::string statement::getFunctionName() const {
      if(type & functionStatementType){
        return (expRoot.leaves[1]->value);
      }

      printf("Not added yet\n");
      throw 1;
      return "";
    }

    void statement::setFunctionName(const std::string &newName){
      if(type & functionStatementType){
        expRoot.leaves[1]->value = newName;
        return;
      }

      printf("Not added yet\n");
      throw 1;
    }

    expNode* statement::getFunctionArgsNode(){
      if(type & functionDefinitionType)
        return expRoot.leaves[2];

      return NULL;
    }

    expNode* statement::getFunctionArgNode(const int pos){
      if(type & functionDefinitionType)
        return getFunctionArgsNode()->leaves[pos];

      return NULL;
    }

    std::string statement::getFunctionArgType(const int pos){
      if(type & functionDefinitionType){
        expNode &arg = *( getFunctionArgNode(pos) );

        if(arg.leaves[0]->info & expType::type){
          expNode &argType = *(arg.leaves[0]);

          if(argType.leaves[0]->info & expType::type)
            return argType.leaves[0]->value;
          else
            return argType.leaves[1]->value;
        }
      }

      return "";
    }

    std::string statement::getFunctionArgName(const int pos){
      if(type & functionDefinitionType){
        expNode &arg = *( getFunctionArgNode(pos) );

        if(arg.info & expType::variable){
          if(arg.leaves[0]->info & expType::variable)
            return arg.leaves[0]->value;
          else
            return arg.leaves[1]->value;
        }
        else if(arg.info & expType::presetValue)
          return arg.value;
      }

      return "";
    }

    varInfo* statement::getFunctionArgVar(const int pos){
      if(type & functionDefinitionType){
        return scopeVarMap[ getFunctionArgName(pos) ];
      }

      return NULL;
    }

    int statement::getFunctionArgCount() const {
      if(type & functionStatementType)
        return (expRoot.leaves[2]->leafCount);

      return 0;
    }


    int statement::getForStatementCount() const {
      if(type & forStatementType)
        return expRoot.leafCount;

      return 0;
    }
    //================================

    // autoMode: Handles newlines and tabs
    std::string statement::prettyString(strNode *nodeRoot,
                                        const std::string &tab_,
                                        const bool autoMode) const {
      strNode *nodePos = nodeRoot;

      std::string tab = tab_;
      std::string ret = "";

      while(nodePos){
        if(nodePos->type & operatorType){

          if(nodePos->type & binaryOperatorType){

            // char *blah
            if(nodeHasQualifier(nodePos)){

              // [char ][*][blah]
              // or
              // [int ][a][ = ][0][, ][*][b][ = ][1][;]
              //                       ^
              if(nodePos->left &&
                 ((nodePos->left->type & descriptorType) ||
                  (nodePos->left->value == ","))){
                ret += *nodePos;

                // [const ][*][ const]
                if(nodePos->right &&
                   (nodePos->right->type & descriptorType) &&
                   !(nodePos->right->value == "*"))
                  ret += ' ';
              }
              else{
                ret += " ";
                ret += *nodePos;
                ret += " ";
              }
            }
            // [+] and [-]
            else if(nodePos->type & unitaryOperatorType){
              // (-blah ... )
              if(nodePos->left &&
                 !(nodePos->left->type & (presetValue | unknownVariable)) )
                ret += *nodePos;
              // a - b
              else{
                ret += " ";
                ret += *nodePos;
                ret += " ";
              }
            }
            else if(nodePos->value == ","){
              ret += ", ";
            }
            else if((nodePos->value == ".") || (nodePos->value == "::")){
              if(((nodePos->left == NULL) ||
                  !(nodePos->left->type & unknownVariable)) ||
                 ((nodePos->right == NULL) ||
                  !(nodePos->right->type & unknownVariable))){
                if(nodePos->left){
                  nodePos->up->print();
                  std::cout << "1. Error on:\n";
                  nodePos->left->print("  ");
                }
                else{
                  std::cout << "2. Error on:\n";
                  nodePos->print("  ");
                }

                throw 1;
              }

              ret += *nodePos;
            }
            else{
              ret += " ";
              ret += *nodePos;
              ret += " ";
            }

          }
          // [++] and [--]
          else if(nodePos->type & unitaryOperatorType){
            ret += *nodePos;
          }
          else if(nodePos->type & ternaryOperatorType){
            ret += " ? ";

            nodePos = nodePos->right;

            if((nodePos->right) == NULL){
              std::cout << "3. Error on:\n";
              nodePos->left->print("  ");
              throw 1;
            }

            if((nodePos->down).size())
              ret += prettyString(nodePos, "", autoMode);
            else
              ret += *nodePos;

            ret += " : ";

            if(((nodePos->right) == NULL)       ||
               ((nodePos->right->right) == NULL)){
              std::cout << "4. Error on:\n";
              nodePos->left->left->print("  ");
              throw 1;
            }

            nodePos = nodePos->right->right;

            if((nodePos->down).size())
              ret += prettyString(nodePos, "", autoMode);
            else
              ret += *nodePos;
          }
        }

        else if(nodePos->type & brace){
          if(nodePos->type & startSection){
            // a[] = {};
            if(nodePos->up->type & binaryOperatorType){
              ret += "{ ";
            }
            else{
              // Case: function(...) const {
              if( (((nodePos->sideDepth) != 0) &&
                   ((nodePos->up->down[nodePos->sideDepth - 1]->type & parentheses) ||
                    (nodePos->up->down[nodePos->sideDepth - 1]->value == "const")) )

                  || (nodePos->up->type & (occaKeywordType | flowControlType)))
                ret += " {\n" + tab + "  ";
              else
                ret += tab + "{\n";
            }

            tab += "  ";
          }
          else{
            tab = tab.substr(0, tab.size() - 2);

            // a[] = {};
            if(nodePos->up &&
               (nodePos->up->type & binaryOperatorType))
              ret += " }";
            else{
              ret += '}';

              //   }
              // }
              if((nodePos->up == NULL) ||
                 ((nodePos->up->right) &&
                  (nodePos->up->right->type == endBrace)))
                ret += "\n" + tab.substr(0, tab.size() - 2);
              else
                ret += "\n" + tab;
            }
          }
        }

        else if(nodePos->type == endParentheses){
          ret += ")";

          // if(...) statement
          if(autoMode)
            if((nodePos->up->type & flowControlType) &&
               (((nodePos->sideDepth) >= (nodePos->up->down.size() - 1)) ||
                !(nodePos->up->down[nodePos->sideDepth + 1]->type & brace))){

              ret += "\n" + tab + "  ";
            }
        }

        else if(nodePos->type & endStatement){
          ret += *nodePos;

          // for(){
          //   ...;
          // }
          if((nodePos->right == NULL) ||
             ((nodePos->right) &&
              (nodePos->right->type & brace))){

            ret += "\n" + tab.substr(0, tab.size() - 2);
          }
          //   blah;
          // }
          else if(!(nodePos->up)                    ||
                  !(nodePos->up->type & flowControlType) ||
                  !(nodePos->up->value == "for")){

            ret += "\n" + tab;
          }
          // Don't add newlines to for(A;B;C)
          else
            ret += " ";
        }

        else if(nodeHasDescriptor(nodePos)){
          ret += *nodePos;

          if(nodePos->right &&
             // [static ][const ][float ][variable]
             ((nodePos->right->type & (presetValue    |
                                       unknownVariable)) ||
              nodeHasDescriptor(nodePos->right))){

            ret += " ";
          }
        }

        else if(nodePos->type & flowControlType){
          ret += *nodePos;

          if(autoMode)
            if(nodePos->down.size() == 0)
              ret += '\n' + tab + "  ";
        }

        else if(nodePos->type & specialKeywordType){
          if(nodePos->value == "case")
            ret += "case";
          else if(nodePos->value == "default")
            ret += "default";
          else if(nodePos->value == "break")
            ret += "break";
          else if(nodePos->value == "continue")
            ret += "continue";
          else if(nodePos->value == "return"){
            ret += "return";

            if(nodePos->right || nodePos->down.size())
              ret += ' ';
          }
          else if(nodePos->value == "goto")
            ret += "goto ";
          else
            ret += *nodePos;
        }
        else if(nodePos->type & macroKeywordType){
          ret += *nodePos;

          ret += '\n' + tab;
        }
        else
          ret += *nodePos;

        const int downCount = (nodePos->down).size();

        for(int i = 0; i < downCount; ++i){
          strNode *downNode = nodePos->down[i];

          ret += prettyString(downNode, tab, autoMode);
        }

        nodePos = nodePos->right;
      }

      return ret;
    }

    statement::operator std::string(){
      const std::string tab = getTab();

      statementNode *statementPos = statementStart;

      // OCCA For's
      if(type == (occaStatementType | forStatementType)){
        std::string ret = expRoot.getString(tab) + " {\n";

        while(statementPos){
          ret += (std::string) *(statementPos->value);
          statementPos = statementPos->right;
        }

        ret += tab + "}\n";

        return ret;
      }

      else if(type & declareStatementType){
        return expRoot.getString(tab);
      }

      else if(type & (simpleStatementType | gotoStatementType)){
        return expRoot.getString(tab) + "\n";
      }

      else if(type & flowStatementType){
        std::string ret = expRoot.getString(tab);

        if(statementCount > 1)
          ret += " {";

        ret += '\n';

        while(statementPos){
          ret += (std::string) *(statementPos->value);
          statementPos = statementPos->right;
        }

        if(statementCount > 1)
          ret += tab + "}\n";

        return ret;
      }

      else if(type & functionStatementType){
        if(type & functionDefinitionType){
          std::string ret = expRoot.getString(tab);

          ret += " {\n";

          while(statementPos){
            ret += (std::string) *(statementPos->value);
            statementPos = statementPos->right;
          }

          ret += tab + "}\n\n";

          return ret;
        }
        else if(type & functionPrototypeType)
          return expRoot.getString(tab);
      }
      else if(type & blockStatementType){
        std::string ret = "";

        if(0 <= depth)
          ret += tab + "{\n";

        while(statementPos){
          ret += (std::string) *(statementPos->value);
          statementPos = statementPos->right;
        }

        if(0 <= depth)
          ret += tab + "}\n";

        return ret;
      }
      else if(type & structStatementType){
        return expRoot.getString(tab);
      }
      else if(type & macroStatementType){
        return expRoot.getString(tab);
      }

      return expRoot.getString(tab);
    }

    std::ostream& operator << (std::ostream &out, statement &s){
      out << (std::string) s;

      return out;
    }
    //==============================================
  };
};