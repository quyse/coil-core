#include "spirv.hpp"
#include <set>
#include <map>
#include <optional>
#include <spirv/unified1/spirv.hpp11>
#include <spirv/unified1/GLSL.std.450.h>

namespace Coil
{
  class SpirvModuleCompiler
  {
  public:
    using ResultId = uint32_t;

    SpirvModuleCompiler()
    {
      // SPIR-V module header
      Emit(_codeModule, spv::MagicNumber);
      Emit(_codeModule, 0x00010000); // SPIR-V 1.0
      Emit(_codeModule, 0x00000000); // code generator magic number
      _upperBoundResultIdOffset = Emit(_codeModule, 0);
      Emit(_codeModule, 0); // reserved for instruction schema

      // allocate extensions result ids ahead of time
      _glslInstSetResultId = _nextResultId++;
    }

    void TraverseEntryPoint(std::string&& name, SpirvStageFlag stage, spv::ExecutionModel executionModel, ShaderStatementNode* node)
    {
      SpirvCode code;
      Function function =
      {
        .stage = stage,
        .executionModel = executionModel,
        .resultId = _nextResultId++,
      };
      TraverseStatement(function, node);
      _functions.insert({ std::move(name), std::move(function) });
    }

    SpirvModule Finalize()
    {
      // emit capabilities
      for(spv::Capability capability : _capabilities)
      {
        EmitOp(_codeModule, spv::Op::OpCapability, [&]()
        {
          Emit(_codeModule, capability);
        });
      }

      // emit extensions
      auto emitExtension = [&](char const* name, ResultId resultId)
      {
        EmitOp(_codeModule, spv::Op::OpExtInstImport, [&]()
        {
          Emit(_codeModule, resultId);
          EmitString(_codeModule, name);
        });
      };
      emitExtension("GLSL.std.450", _glslInstSetResultId);

      // emit memory model
      EmitOp(_codeModule, spv::Op::OpMemoryModel, [&]()
      {
        Emit(_codeModule, spv::AddressingModel::Logical);
        Emit(_codeModule, spv::MemoryModel::Simple);
      });

      // emit entry points
      for(auto const& function : _functions)
      {
        EmitOp(_codeModule, spv::Op::OpEntryPoint, [&]()
        {
          Emit(_codeModule, function.second.executionModel);
          Emit(_codeModule, function.second.resultId);
          EmitString(_codeModule, function.first.c_str());
          for(auto variableResultId : function.second.interfaceVariablesResultIds)
            Emit(_codeModule, variableResultId);
        });
      }

      // emit extra entry point declarations
      for(auto const& function : _functions)
      {
        if(function.second.executionModel == spv::ExecutionModel::Fragment)
        {
          EmitOp(_codeModule, spv::Op::OpExecutionMode, [&]()
          {
            Emit(_codeModule, function.second.resultId);
            Emit(_codeModule, spv::ExecutionMode::OriginUpperLeft); // upper-left is actually required by Vulkan spec
          });
        }
      }

      // append annotations
      _codeModule.insert(_codeModule.end(), _codeAnnotations.begin(), _codeAnnotations.end());

      // all functions use type void(), emit it
      ResultId voidTypeResultId;
      EmitOp(_codeDecls, spv::Op::OpTypeVoid, [&]()
      {
        voidTypeResultId = EmitResultId(_codeDecls);
      });
      ResultId functionTypeResultId;
      EmitOp(_codeDecls, spv::Op::OpTypeFunction, [&]()
      {
        functionTypeResultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, voidTypeResultId);
      });

      // append declarations
      _codeModule.insert(_codeModule.end(), _codeDecls.begin(), _codeDecls.end());

      // emit functions
      for(auto const& it : _functions)
      {
        auto const& function = it.second;

        // header
        EmitOp(_codeModule, spv::Op::OpFunction, [&]()
        {
          Emit(_codeModule, voidTypeResultId);
          Emit(_codeModule, function.resultId);
          Emit(_codeModule, spv::FunctionControlMask::MaskNone);
          Emit(_codeModule, functionTypeResultId);
        });

        // begin label
        ResultId beginLabelResultId;
        EmitOp(_codeModule, spv::Op::OpLabel, [&]()
        {
          beginLabelResultId = EmitResultId(_codeModule);
        });

        // body
        _codeModule.insert(_codeModule.end(), function.code.begin(), function.code.end());

        // end
        EmitOp(_codeModule, spv::Op::OpReturn, [&]() {});
        EmitOp(_codeModule, spv::Op::OpFunctionEnd, [&]() {});
      }

      // fix up result id upper bound
      _codeModule[_upperBoundResultIdOffset] = _nextResultId;

      // prepare module
      SpirvModule module =
      {
        .code = std::move(_codeModule),
      };

      // generate descriptor layout
      if(!_bindings.empty())
      {
        module.descriptorSetLayouts.resize(_bindings.rbegin()->first + 1);
        for(auto const& descriptorSetIt : _bindings)
        {
          SpirvDescriptorSetLayout& descriptorSetLayout = module.descriptorSetLayouts[descriptorSetIt.first];
          if(!descriptorSetIt.second.empty())
          {
            descriptorSetLayout.bindings.resize(descriptorSetIt.second.rbegin()->first + 1);

            for(auto const& descriptorBindingIt : descriptorSetIt.second)
              descriptorSetLayout.bindings[descriptorBindingIt.first] = descriptorBindingIt.second;
          }
        }
      }

      return std::move(module);
    }

  private:
    struct Function
    {
      SpirvStageFlag stage;
      spv::ExecutionModel executionModel;
      ResultId resultId;
      std::map<ShaderExpressionNode*, ResultId> expressionResultIds;
      std::map<std::pair<ShaderVariableNode*, spv::StorageClass>, ResultId> variablesResultIds;
      std::set<ResultId> interfaceVariablesResultIds;
      SpirvCode code;
    };

    ResultId TraverseExpression(Function& function, ShaderExpressionNode* node)
    {
      auto it = function.expressionResultIds.find(node);
      if(it != function.expressionResultIds.end()) return it->second;
      // insert first with zero id, change it later
      ResultId resultId = 0;
      it = function.expressionResultIds.insert({ node, resultId }).first;

      ShaderDataType const& dataType = node->GetDataType();
      ResultId typeResultId = TraverseType(dataType);

      switch(node->GetExpressionType())
      {
      case ShaderExpressionType::Operation:
        {
          ShaderOperationNode* operationNode = static_cast<ShaderOperationNode*>(node);

          // traverse args
          size_t argsCount = operationNode->GetArgsCount();
          ResultId* argsResultIds = (ResultId*)alloca(argsCount * sizeof(ResultId));
          for(size_t i = 0; i < argsCount; ++i)
            argsResultIds[i] = TraverseExpression(function, operationNode->GetArg(i).get());

          // operation types
          switch(operationNode->GetOperationType())
          {
          case ShaderOperationType::Const:
            switch(dataType.GetKind())
            {
            case ShaderDataKind::Scalar:
              switch(static_cast<ShaderDataScalarType const&>(dataType).type)
              {
#define S(T, t) \
              case ShaderDataScalarType::Type::_##t: \
                resultId = TraverseConst(static_cast<ShaderOperationConstNode<T>*>(operationNode)->value); \
                break

              S(float, float);
              S(uint32_t, uint);
              S(int32_t, int);
              S(bool, bool);
#undef S
              default:
                throw Exception("unsupported SPIR-V shader data scalar type for constant scalar");
              }
              break;
            case ShaderDataKind::Vector:
              {
                ShaderDataVectorType const& dataVectorType = static_cast<ShaderDataVectorType const&>(dataType);
#define S(T, n) \
                  case n: \
                    resultId = TraverseCompositeConst(dataType, static_cast<ShaderOperationConstNode<xvec<T, n>>*>(operationNode)->value); \
                    break
#define Q(T, t) \
                case ShaderDataScalarType::Type::_##t: \
                  switch(dataVectorType.n) \
                  { \
                  S(T, 2); \
                  S(T, 3); \
                  S(T, 4); \
                  default: \
                    throw Exception("unsupported SPIR-V shader data size for constant vector of " #t "s"); \
                  } \
                  break

                switch(dataVectorType.baseType.type)
                {
                Q(float, float);
                Q(uint32_t, uint);
                Q(int32_t, int);
                Q(bool, bool);
                default:
                  throw Exception("unsupported SPIR-V shader data base type for constant vector");
                }
#undef Q
#undef S
              }
              break;
            case ShaderDataKind::Matrix:
              {
                ShaderDataMatrixType const& dataMatrixType = static_cast<ShaderDataMatrixType const&>(dataType);
#define S(T, n, m) \
                  case ((n << 16) | m): \
                    resultId = TraverseCompositeConst(dataType, static_cast<ShaderOperationConstNode<xmat<T, n, m>>*>(operationNode)->value); \
                    break
#define Q(T, t) \
                case ShaderDataScalarType::Type::_##t: \
                  switch((dataMatrixType.n << 16) | dataMatrixType.m) \
                  { \
                  S(T, 1, 2); \
                  S(T, 1, 3); \
                  S(T, 1, 4); \
                  S(T, 2, 1); \
                  S(T, 2, 2); \
                  S(T, 2, 3); \
                  S(T, 2, 4); \
                  S(T, 3, 1); \
                  S(T, 3, 2); \
                  S(T, 3, 3); \
                  S(T, 3, 4); \
                  S(T, 4, 1); \
                  S(T, 4, 2); \
                  S(T, 4, 3); \
                  S(T, 4, 4); \
                  default: \
                    throw Exception("unsupported SPIR-V shader data size for constant matrix of " #t "s"); \
                  } \
                  break

                switch(dataMatrixType.baseType.type)
                {
                Q(float, float);
                Q(uint32_t, uint);
                Q(int32_t, int);
                Q(bool, bool);
                default:
                  throw Exception("unsupported SPIR-V shader data base type for constant matrix");
                }
#undef Q
#undef S
              }
              break;
            default:
              throw Exception("unsupported SPIR-V shader data kind for constant");
            }
            break;
          case ShaderOperationType::Construct:
            switch(dataType.GetKind())
            {
            case ShaderDataKind::Vector:
              EmitOp(function.code, spv::Op::OpCompositeConstruct, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                for(size_t i = 0; i < argsCount; ++i)
                  Emit(function.code, argsResultIds[i]);
              });
              break;
            default:
              throw Exception("unsupported SPIR-V shader data kind for construction");
            }
            break;
          case ShaderOperationType::Swizzle:
            {
              ShaderOperationSwizzleNode* swizzleNode = static_cast<ShaderOperationSwizzleNode*>(operationNode);
              bool singleComponent = swizzleNode->GetDataType().GetKind() == ShaderDataKind::Scalar;
              EmitOp(function.code, singleComponent ? spv::Op::OpCompositeExtract : spv::Op::OpVectorShuffle, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
                if(!singleComponent) Emit(function.code, argsResultIds[0]);
                for(size_t i = 0; swizzleNode->swizzle[i]; ++i)
                {
                  uint32_t component;
                  switch(swizzleNode->swizzle[i])
                  {
                  case 'x':
                  case 'r':
                    component = 0;
                    break;
                  case 'y':
                  case 'g':
                    component = 1;
                    break;
                  case 'z':
                  case 'b':
                    component = 2;
                    break;
                  case 'w':
                  case 'a':
                    component = 3;
                    break;
                  default:
                    throw Exception("invalid SPIR-V shader swizzle");
                  }
                  Emit(function.code, component);
                }
              });
            }
            break;
          // case ShaderOperationType::Index:
          //   {
          //   }
          //   break;
          case ShaderOperationType::Negate:
            switch(ShaderDataTypeGetScalarType(dataType).type)
            {
            case ShaderDataScalarType::Type::_float:
              EmitOp(function.code, spv::Op::OpFNegate, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
              });
              break;
            case ShaderDataScalarType::Type::_uint:
            case ShaderDataScalarType::Type::_int:
              EmitOp(function.code, spv::Op::OpSNegate, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
              });
              break;
            default:
              throw Exception("unsupported SPIR-V shader component type for scalar/vector negation");
            }
            break;
          case ShaderOperationType::Add:
          case ShaderOperationType::Subtract:
            switch(ShaderDataTypeGetScalarType(dataType).type)
            {
            case ShaderDataScalarType::Type::_float:
              EmitOp(function.code,
                operationNode->GetOperationType() == ShaderOperationType::Add ?
                  spv::Op::OpFAdd :
                  spv::Op::OpFSub,
                [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
                Emit(function.code, argsResultIds[1]);
              });
              break;
            case ShaderDataScalarType::Type::_uint:
            case ShaderDataScalarType::Type::_int:
              EmitOp(function.code,
                operationNode->GetOperationType() == ShaderOperationType::Add ?
                  spv::Op::OpIAdd :
                  spv::Op::OpISub,
                [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
                Emit(function.code, argsResultIds[1]);
              });
              break;
            default:
              throw Exception("unsupported SPIR-V shader component type for vector addition/subtraction");
            }
            break;
          case ShaderOperationType::Multiply:
            switch(ShaderDataTypeGetScalarType(dataType).type)
            {
            case ShaderDataScalarType::Type::_float:
              EmitOp(function.code, spv::Op::OpFMul, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
                Emit(function.code, argsResultIds[1]);
              });
              break;
            case ShaderDataScalarType::Type::_uint:
            case ShaderDataScalarType::Type::_int:
              EmitOp(function.code, spv::Op::OpIMul, [&]()
              {
                Emit(function.code, typeResultId);
                resultId = EmitResultId(function.code);
                Emit(function.code, argsResultIds[0]);
                Emit(function.code, argsResultIds[1]);
              });
              break;
            default:
              throw Exception("unsupported SPIR-V shader data scalar type for multiplication");
            }
            break;
          // case ShaderOperationType::Divide:
          default:
            throw Exception("unknown SPIR-V shader operation type");
          }
        }
        break;
      case ShaderExpressionType::Read:
        {
          ShaderReadNode* readNode = static_cast<ShaderReadNode*>(node);
          ResultId variableResultId = TraverseVariable(function, readNode->node.get());
          EmitOp(function.code, spv::Op::OpLoad, [&]()
          {
            Emit(function.code, typeResultId);
            resultId = EmitResultId(function.code);
            Emit(function.code, variableResultId);
          });
        }
        break;
      case ShaderExpressionType::Uniform:
        {
          ShaderUniformNode* uniformNode = static_cast<ShaderUniformNode*>(node);
          ResultId typeResultId = TraverseType(uniformNode->dataType);
          ResultId ptrTypeResultId = TraversePointerType(uniformNode->dataType, spv::StorageClass::Uniform);
          ShaderUniformBufferNode* uniformBufferNode = uniformNode->uniformBufferNode.get();
          ResultId uniformBufferResultId = TraverseUniformBuffer(uniformBufferNode);
          ResultId indexResultId = TraverseConst(uniformNode->index);
          ResultId ptrResultId;
          EmitOp(function.code, spv::Op::OpAccessChain, [&]()
          {
            Emit(function.code, ptrTypeResultId);
            ptrResultId = EmitResultId(function.code);
            Emit(function.code, uniformBufferResultId);
            Emit(function.code, indexResultId);
          });
          EmitOp(function.code, spv::Op::OpLoad, [&]()
          {
            Emit(function.code, typeResultId);
            resultId = EmitResultId(function.code);
            Emit(function.code, ptrResultId);
          });

          _bindings[uniformBufferNode->slotSet][uniformBufferNode->slot].stageFlags |= (uint32_t)function.stage;
        }
        break;
      default:
        throw Exception("unknown SPIR-V shader expression type");
      }

      it->second = resultId;

      return resultId;
    }

    void TraverseStatement(Function& function, ShaderStatementNode* node)
    {
      switch(node->GetStatementType())
      {
      case ShaderStatementType::Sequence:
        {
          ShaderStatementSequenceNode* sequenceNode = static_cast<ShaderStatementSequenceNode*>(node);
          TraverseStatement(function, sequenceNode->a.get());
          TraverseStatement(function, sequenceNode->b.get());
        }
        break;
      case ShaderStatementType::Write:
        {
          ShaderStatementWriteNode* writeNode = static_cast<ShaderStatementWriteNode*>(node);
          ResultId variableResultId = TraverseVariable(function, writeNode->variableNode.get());
          ResultId expressionResultId = TraverseExpression(function, writeNode->expressionNode.get());
          EmitOp(function.code, spv::Op::OpStore, [&]()
          {
            Emit(function.code, variableResultId);
            Emit(function.code, expressionResultId);
          });
        }
        break;
      default:
        throw Exception("unknown SPIR-V shader statement type");
      }
    }

    ResultId TraverseVariable(Function& function, ShaderVariableNode* node)
    {
      spv::StorageClass storageClass;
      uint32_t location;
      std::optional<spv::BuiltIn> builtin;

      switch(node->GetVariableType())
      {
      case ShaderVariableType::Attribute:
        {
          ShaderVariableAttributeNode* attributeNode = static_cast<ShaderVariableAttributeNode*>(node);
          location = attributeNode->location;
          switch(attributeNode->builtin)
          {
          case ShaderAttributeBuiltin::None:
            break;
          case ShaderAttributeBuiltin::VertexIndex:
            builtin = spv::BuiltIn::VertexIndex;
            break;
          case ShaderAttributeBuiltin::InstanceIndex:
            builtin = spv::BuiltIn::InstanceIndex;
            break;
          default:
            throw Exception("unknown SPIR-V shader attribute builtin");
          }
        }
        switch(function.executionModel)
        {
        case spv::ExecutionModel::Vertex:
          storageClass = spv::StorageClass::Input;
          break;
        default:
          throw Exception("unsupported SPIR-V execution model for shader attribute variable");
        }
        break;
      case ShaderVariableType::Interpolant:
        {
          ShaderVariableInterpolantNode* interpolantNode = static_cast<ShaderVariableInterpolantNode*>(node);
          location = interpolantNode->location;
          switch(interpolantNode->builtin)
          {
          case ShaderInterpolantBuiltin::None:
            break;
          case ShaderInterpolantBuiltin::Position:
            builtin = spv::BuiltIn::Position;
            break;
          case ShaderInterpolantBuiltin::PointSize:
            builtin = spv::BuiltIn::PointSize;
            break;
          case ShaderInterpolantBuiltin::FragCoord:
            builtin = spv::BuiltIn::FragCoord;
            break;
          default:
            throw Exception("unknown SPIR-V shader interpolant builtin");
          }
        }
        switch(function.executionModel)
        {
        case spv::ExecutionModel::Vertex:
          storageClass = spv::StorageClass::Output;
          break;
        case spv::ExecutionModel::Fragment:
          storageClass = spv::StorageClass::Input;
          break;
        default:
          throw Exception("unsupported SPIR-V execution model for shader attribute variable");
        }
        break;
      case ShaderVariableType::Fragment:
        {
          ShaderVariableFragmentNode* fragmentNode = static_cast<ShaderVariableFragmentNode*>(node);
          location = fragmentNode->location;
          switch(fragmentNode->builtin)
          {
          case ShaderFragmentBuiltin::None:
            break;
          case ShaderFragmentBuiltin::FragDepth:
            builtin = spv::BuiltIn::FragDepth;
            break;
          default:
            throw Exception("unknown SPIR-V shader fragment builtin");
          }
        }
        switch(function.executionModel)
        {
        case spv::ExecutionModel::Fragment:
          storageClass = spv::StorageClass::Output;
          break;
        default:
          throw Exception("unsupported SPIR-V execution model for shader fragment variable");
        }
        break;
      default:
        throw Exception("unknown SPIR-V shader variable type");
      }

      auto it = function.variablesResultIds.find({ node, storageClass });
      if(it != function.variablesResultIds.end()) return it->second;
      it = function.variablesResultIds.insert({ { node, storageClass }, 0 }).first;

      ShaderDataType const& dataType = node->GetDataType();
      ResultId typeResultId = TraversePointerType(dataType, storageClass);
      ResultId resultId;

      EmitOp(_codeDecls, spv::Op::OpVariable, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, storageClass);
      });

      it->second = resultId;

      if(builtin.has_value())
      {
        EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
        {
          Emit(_codeAnnotations, resultId);
          Emit(_codeAnnotations, spv::Decoration::BuiltIn);
          Emit(_codeAnnotations, builtin.value());
        });
      }
      else
      {
        EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
        {
          Emit(_codeAnnotations, resultId);
          Emit(_codeAnnotations, spv::Decoration::Location);
          Emit(_codeAnnotations, location);
        });
      }

      function.interfaceVariablesResultIds.insert(resultId);

      return resultId;
    }

    ResultId TraverseType(ShaderDataType const& dataType)
    {
      auto it = _typeResultIds.find(&dataType);
      if(it != _typeResultIds.end()) return it->second;
      it = _typeResultIds.insert({ &dataType, 0 }).first;

      ResultId resultId;

      switch(dataType.GetKind())
      {
      case ShaderDataKind::Scalar:
        switch(static_cast<ShaderDataScalarType const&>(dataType).type)
        {
        case ShaderDataScalarType::Type::_float:
          EmitOp(_codeDecls, spv::Op::OpTypeFloat, [&]()
          {
            resultId = EmitResultId(_codeDecls);
            Emit(_codeDecls, 32);
          });
          break;
        case ShaderDataScalarType::Type::_uint:
          EmitOp(_codeDecls, spv::Op::OpTypeInt, [&]()
          {
            resultId = EmitResultId(_codeDecls);
            Emit(_codeDecls, 32);
            Emit(_codeDecls, 0);
          });
          break;
        case ShaderDataScalarType::Type::_int:
          EmitOp(_codeDecls, spv::Op::OpTypeInt, [&]()
          {
            resultId = EmitResultId(_codeDecls);
            Emit(_codeDecls, 32);
            Emit(_codeDecls, 1);
          });
          break;
        case ShaderDataScalarType::Type::_bool:
          EmitOp(_codeDecls, spv::Op::OpTypeBool, [&]()
          {
            resultId = EmitResultId(_codeDecls);
          });
          break;
        default:
          throw Exception("unsupported SPIR-V shader scalar data type");
        }
        break;
      case ShaderDataKind::Vector:
        {
          ShaderDataVectorType const& dataVectorType = static_cast<ShaderDataVectorType const&>(dataType);
          resultId = TraverseCompositeType(spv::Op::OpTypeVector, dataVectorType.baseType, dataVectorType.n);
        }
        break;
      case ShaderDataKind::Matrix:
        {
          ShaderDataMatrixType const& dataMatrixType = static_cast<ShaderDataMatrixType const&>(dataType);
          resultId = TraverseCompositeType(spv::Op::OpTypeMatrix,
            ShaderDataVectorType(
              dataMatrixType.baseType,
              dataMatrixType.n,
              // does not account for alignment, but column's size is not used anywhere
              dataMatrixType.n * dataMatrixType.baseType.GetSize()
            ),
            dataMatrixType.m);
        }
        break;
      case ShaderDataKind::Array:
        {
          ShaderDataArrayType const& dataArrayType = static_cast<ShaderDataArrayType const&>(dataType);
          resultId = TraverseCompositeType(spv::Op::OpTypeArray, dataArrayType.baseType, dataArrayType.n);
        }
        break;
      case ShaderDataKind::Struct:
        {
          ShaderDataStructType const& dataStructType = static_cast<ShaderDataStructType const&>(dataType);
          size_t membersCount = dataStructType.members.size();

          // members types
          ResultId* membersTypeResultIds = (ResultId*)alloca(membersCount * sizeof(ResultId));
          for(size_t i = 0; i < membersCount; ++i)
            membersTypeResultIds[i] = TraverseType(dataStructType.members[i].second);

          // struct type
          EmitOp(_codeDecls, spv::Op::OpTypeStruct, [&]()
          {
            resultId = EmitResultId(_codeDecls);
            for(size_t i = 0; i < membersCount; ++i)
              Emit(_codeDecls, membersTypeResultIds[i]);
          });

          // struct decorations
          EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
          {
            Emit(_codeAnnotations, resultId);
            Emit(_codeAnnotations, spv::Decoration::Block);
          });

          // members decorations
          for(size_t i = 0; i < membersCount; ++i)
          {
            EmitOp(_codeAnnotations, spv::Op::OpMemberDecorate, [&]()
            {
              Emit(_codeAnnotations, resultId);
              Emit(_codeAnnotations, (uint32_t)i);
              Emit(_codeAnnotations, spv::Decoration::Offset);
              Emit(_codeAnnotations, dataStructType.members[i].first);
            });

            ShaderDataType const& memberDataType = dataStructType.members[i].second;

            switch(memberDataType.GetKind())
            {
            case ShaderDataKind::Scalar:
              break;
            case ShaderDataKind::Vector:
              break;
            case ShaderDataKind::Matrix:
              {
                ShaderDataMatrixType const& memberMatrixDataType = static_cast<ShaderDataMatrixType const&>(memberDataType);

                EmitOp(_codeAnnotations, spv::Op::OpMemberDecorate, [&]()
                {
                  Emit(_codeAnnotations, resultId);
                  Emit(_codeAnnotations, (uint32_t)i);
                  Emit(_codeAnnotations, spv::Decoration::RowMajor);
                });
                EmitOp(_codeAnnotations, spv::Op::OpMemberDecorate, [&]()
                {
                  Emit(_codeAnnotations, resultId);
                  Emit(_codeAnnotations, (uint32_t)i);
                  Emit(_codeAnnotations, spv::Decoration::MatrixStride);
                  Emit(_codeAnnotations, memberMatrixDataType.rowStride);
                });
              }
              break;
            case ShaderDataKind::Array:
              {
                ShaderDataArrayType const& memberArrayDataType = static_cast<ShaderDataArrayType const&>(memberDataType);

                EmitOp(_codeAnnotations, spv::Op::OpMemberDecorate, [&]()
                {
                  Emit(_codeAnnotations, resultId);
                  Emit(_codeAnnotations, (uint32_t)i);
                  Emit(_codeAnnotations, spv::Decoration::ArrayStride);
                  Emit(_codeAnnotations, memberArrayDataType.baseType.GetSize());
                });
              }
              break;
            case ShaderDataKind::Struct:
              break;
            }
          }
        }
        break;
      default:
        throw Exception("unsupported SPIR-V shader data kind");
      }

      it->second = resultId;

      return resultId;
    }
    ResultId TraverseCompositeType(spv::Op op, ShaderDataType const& componentDataType, size_t componentsCount)
    {
      ResultId componentResultId = TraverseType(componentDataType);
      ResultId resultId;
      EmitOp(_codeDecls, op, [&]()
      {
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, componentResultId);
        Emit(_codeDecls, componentsCount);
      });
      return resultId;
    }
    ResultId TraversePointerType(ShaderDataType const& dataType, spv::StorageClass storageClass)
    {
      auto it = _pointerTypeResultIds.find({ &dataType, storageClass });
      if(it != _pointerTypeResultIds.end()) return it->second;
      it = _pointerTypeResultIds.insert({ { &dataType, storageClass }, 0 }).first;

      ResultId typeResultId = TraverseType(dataType);

      ResultId resultId;

      EmitOp(_codeDecls, spv::Op::OpTypePointer, [&]()
      {
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, storageClass);
        Emit(_codeDecls, typeResultId);
      });

      it->second = resultId;

      return resultId;
    }

    template <typename T>
    ResultId TraverseConst(T value);
    template <>
    ResultId TraverseConst(float value)
    {
      ResultId typeResultId = TraverseType(ShaderDataTypeOf<float>());
      auto it = _floatConstResultIds.find(value);
      if(it != _floatConstResultIds.end()) return it->second;
      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpConstant, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, *(uint32_t*)&value);
      });
      _floatConstResultIds.insert({ value, resultId });
      return resultId;
    }
    template <>
    ResultId TraverseConst(uint32_t value)
    {
      ResultId typeResultId = TraverseType(ShaderDataTypeOf<uint32_t>());
      auto it = _uintConstResultIds.find(value);
      if(it != _uintConstResultIds.end()) return it->second;
      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpConstant, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, value);
      });
      _uintConstResultIds.insert({ value, resultId });
      return resultId;
    }
    template <>
    ResultId TraverseConst(int32_t value)
    {
      ResultId typeResultId = TraverseType(ShaderDataTypeOf<int32_t>());
      auto it = _intConstResultIds.find(value);
      if(it != _intConstResultIds.end()) return it->second;
      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpConstant, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, value);
      });
      _intConstResultIds.insert({ value, resultId });
      return resultId;
    }
    template <>
    ResultId TraverseConst(bool value)
    {
      ResultId typeResultId = TraverseType(ShaderDataTypeOf<bool>());
      auto it = _boolConstResultIds.find(value);
      if(it != _boolConstResultIds.end()) return it->second;
      ResultId resultId;
      EmitOp(_codeDecls, value ? spv::Op::OpConstantTrue : spv::Op::OpConstantFalse, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
      });
      _boolConstResultIds.insert({ value, resultId });
      return resultId;
    }

    template <typename T, size_t n>
    ResultId TraverseCompositeConst(ShaderDataType const& dataType, xvec<T, n> const& value)
    {
      ResultId typeResultId = TraverseType(dataType);
      ResultId valueResultIds[n];
      for(size_t i = 0; i < n; ++i)
        valueResultIds[i] = TraverseConst(value.t[i]);
      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpConstantComposite, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        for(size_t i = 0; i < n; ++i)
          Emit(_codeDecls, valueResultIds[i]);
      });
      return resultId;
    }
    template <typename T, size_t n, size_t m>
    ResultId TraverseCompositeConst(ShaderDataType const& dataType, xmat<T, n, m> const& value)
    {
      ResultId typeResultId = TraverseType(dataType);
      ResultId valueResultIds[m];
      for(size_t j = 0; j < m; ++j)
      {
        xvec<T, n> column;
        for(size_t i = 0; i < n; ++i)
          column.t[i] = value(i, j);
        valueResultIds[j] = TraverseCompositeConst(ShaderDataTypeOf<xvec<T, n>>(), column);
      }
      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpConstantComposite, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        for(size_t j = 0; j < m; ++j)
          Emit(_codeDecls, valueResultIds[j]);
      });
      return resultId;
    }

    ResultId TraverseUniformBuffer(ShaderUniformBufferNode* node)
    {
      auto it = _uniformBufferResultIds.find(node);
      if(it != _uniformBufferResultIds.end()) return it->second;

      ResultId typeResultId = TraversePointerType(node->dataType, spv::StorageClass::Uniform);

      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpVariable, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, spv::StorageClass::Uniform);
      });

      EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
      {
        Emit(_codeAnnotations, resultId);
        Emit(_codeAnnotations, spv::Decoration::DescriptorSet);
        Emit(_codeAnnotations, node->slotSet);
      });
      EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
      {
        Emit(_codeAnnotations, resultId);
        Emit(_codeAnnotations, spv::Decoration::Binding);
        Emit(_codeAnnotations, node->slot);
      });

      _uniformBufferResultIds.insert({ node, resultId });

      SpirvDescriptorSetLayoutBinding& binding = _bindings[node->slotSet][node->slot];
      if(binding.descriptorType != SpirvDescriptorType::UniformBuffer)
      {
        if(binding.descriptorType != SpirvDescriptorType::Unused)
          throw Exception("conflicting SPIR-V descriptor for uniform buffer");
        binding.descriptorType = SpirvDescriptorType::UniformBuffer;
        binding.descriptorCount = std::max<uint32_t>(binding.descriptorCount, 1);
      }

      return resultId;
    }

    static size_t Emit(SpirvCode& code, auto word)
    {
      size_t offset = code.size();
      code.push_back((uint32_t)word);
      return offset;
    }
    static void EmitOp(SpirvCode& code, spv::Op op, auto const& f)
    {
      size_t offset = Emit(code, op);
      f();
      code[offset] |= (code.size() - offset) << 16;
    }
    ResultId EmitResultId(SpirvCode& code)
    {
      uint32_t result = _nextResultId++;
      Emit(code, result);
      return result;
    };
    static void EmitString(SpirvCode& code, char const* str)
    {
      for(size_t i = 0; ; ++i)
      {
        uint32_t word = 0;
        // ensure little endian packing, as per spec
        size_t j;
        for(j = 0; j < 4; ++j)
        {
          uint32_t c = (uint8_t)str[i * 4 + j];
          word |= c << (j * 8);
          if(!c) break;
        }
        Emit(code, word);
        if(j < 4) break;
      }
    };

    // comparator for maps to corrently sort data type pointers by value
    struct ShaderDataTypePtrComparator
    {
      bool operator()(ShaderDataType const* a, ShaderDataType const* b) const
      {
        return *a < *b;
      }

      bool operator()(std::pair<ShaderDataType const*, spv::StorageClass> const& a, std::pair<ShaderDataType const*, spv::StorageClass> const& b) const
      {
        if(*a.first < *b.first) return true;
        if(*b.first < *a.first) return false;
        return a.second < b.second;
      }
    };

    SpirvCode _codeAnnotations; // annotations
    SpirvCode _codeDecls; // types, constants, global variables
    SpirvCode _codeModule; // module code
    std::map<std::string, Function> _functions; // entry points
    std::map<ShaderDataType const*, ResultId, ShaderDataTypePtrComparator> _typeResultIds;
    std::map<std::pair<ShaderDataType const*, spv::StorageClass>, ResultId, ShaderDataTypePtrComparator> _pointerTypeResultIds;
    std::map<float, ResultId> _floatConstResultIds;
    std::map<uint32_t, ResultId> _uintConstResultIds;
    std::map<int32_t, ResultId> _intConstResultIds;
    std::map<bool, ResultId> _boolConstResultIds; // yeah, stupid
    std::map<ShaderUniformBufferNode*, ResultId> _uniformBufferResultIds;
    std::set<spv::Capability> _capabilities =
    {
      spv::Capability::Shader, // enables Matrix as well
    };
    ResultId _nextResultId = 1;
    uint32_t _upperBoundResultIdOffset = 0;
    ResultId _glslInstSetResultId = 0;
    std::map<uint32_t, std::map<uint32_t, SpirvDescriptorSetLayoutBinding>> _bindings;
  };

  SpirvModule SpirvCompile(GraphicsShaderRoots const& roots)
  {
    SpirvModuleCompiler compiler;

    if(roots.vertex)
      compiler.TraverseEntryPoint("mainVertex", SpirvStageFlag::Vertex, spv::ExecutionModel::Vertex, roots.vertex.get());
    if(roots.fragment)
      compiler.TraverseEntryPoint("mainFragment", SpirvStageFlag::Fragment, spv::ExecutionModel::Fragment, roots.fragment.get());

    return compiler.Finalize();
  }
}
