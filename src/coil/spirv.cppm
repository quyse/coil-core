module;

#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <spirv/unified1/spirv.hpp11>
#include <spirv/unified1/GLSL.std.450.h>

export module coil.core.spirv;

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.math;

export namespace Coil
{
  using SpirvCode = std::vector<uint32_t>;

  enum class SpirvDescriptorType
  {
    Unused,
    UniformBuffer,
    StorageBuffer,
    SampledImage,
  };

  enum class SpirvStageFlag
  {
    Vertex = 1,
    TessellationControl = 2,
    TessellationEvaluation = 4,
    Fragment = 8,
    Compute = 16,
  };

  struct SpirvDescriptorSetLayoutBinding
  {
    SpirvDescriptorType descriptorType = SpirvDescriptorType::Unused;
    uint32_t descriptorCount = 0;
    uint32_t stageFlags = 0;
  };

  struct SpirvDescriptorSetLayout
  {
    std::vector<SpirvDescriptorSetLayoutBinding> bindings;
  };

  struct SpirvModule
  {
    SpirvCode code;
    std::vector<SpirvDescriptorSetLayout> descriptorSetLayouts;
  };

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

      switch(executionModel)
      {
      case spv::ExecutionModel::Vertex:
      case spv::ExecutionModel::Fragment:
      case spv::ExecutionModel::GLCompute:
        _capabilities.insert(spv::Capability::Shader);
        break;
      case spv::ExecutionModel::TessellationControl:
      case spv::ExecutionModel::TessellationEvaluation:
        _capabilities.insert(spv::Capability::Tessellation);
        break;
      default:
        break;
      }
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
      for(auto extension : _extensions)
      {
        EmitOp(_codeModule, spv::Op::OpExtension, [&]()
        {
          EmitString(_codeModule, extension);
        });
      }
      // emit instruction extensions
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
        switch(function.second.executionModel)
        {
        case spv::ExecutionModel::TessellationControl:
          EmitOp(_codeModule, spv::Op::OpExecutionMode, [&]()
          {
            Emit(_codeModule, function.second.resultId);
            Emit(_codeModule, spv::ExecutionMode::OutputVertices);
            Emit(_codeModule, _tessellationOutputVertices);
          });
          break;
        case spv::ExecutionModel::Fragment:
          EmitOp(_codeModule, spv::Op::OpExecutionMode, [&]()
          {
            Emit(_codeModule, function.second.resultId);
            Emit(_codeModule, spv::ExecutionMode::OriginUpperLeft); // upper-left is actually required by Vulkan spec
          });
          break;
        case spv::ExecutionModel::GLCompute:
          EmitOp(_codeModule, spv::Op::OpExecutionMode, [&]()
          {
            Emit(_codeModule, function.second.resultId);
            Emit(_codeModule, spv::ExecutionMode::LocalSize);
            Emit(_codeModule, _computeSize.x());
            Emit(_codeModule, _computeSize.y());
            Emit(_codeModule, _computeSize.z());
          });
          break;
        default:
          break;
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

    void SetTessellationOutputVertices(uint32_t tessellationOutputVertices)
    {
      _tessellationOutputVertices = tessellationOutputVertices;
    }

    void SetComputeSize(ivec3 computeSize)
    {
      _computeSize = computeSize;
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

          auto emitSimpleOp = [&](spv::Op op)
          {
            EmitOp(function.code, op, [&]()
            {
              Emit(function.code, typeResultId);
              resultId = EmitResultId(function.code);
              for(size_t i = 0; i < argsCount; ++i)
                Emit(function.code, argsResultIds[i]);
            });
          };

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
              emitSimpleOp(spv::Op::OpCompositeConstruct);
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

// helper macros for ops
#define OP_SCALAR(op, body) \
          case ShaderOperationType::op: \
            switch(ShaderDataTypeGetScalarType(dataType).type) \
            { \
            body \
            default: \
              throw Exception("unsupported SPIR-V shader data scalar type for operation " #op); \
            } \
            break
#define SCALAR_SOP(scalarType, sop) \
            case ShaderDataScalarType::Type::_##scalarType: \
              emitSimpleOp(spv::Op::Op##sop); \
              break

          OP_SCALAR(Negate,
            SCALAR_SOP(float, FNegate);
            SCALAR_SOP(uint, SNegate);
            SCALAR_SOP(int, SNegate);
          );
          OP_SCALAR(Add,
            SCALAR_SOP(float, FAdd);
            SCALAR_SOP(uint, IAdd);
            SCALAR_SOP(int, IAdd);
          );
          OP_SCALAR(Subtract,
            SCALAR_SOP(float, FSub);
            SCALAR_SOP(uint, ISub);
            SCALAR_SOP(int, ISub);
          );

          // multiply is complicated
          case ShaderOperationType::Multiply:
            {
              ShaderDataKind kind0 = operationNode->GetArg(0)->GetDataType().GetKind();
              ShaderDataKind kind1 = operationNode->GetArg(1)->GetDataType().GetKind();

              // scalar-scalar or vector-vector
              if(kind0 == kind1 && (kind0 == ShaderDataKind::Scalar || kind0 == ShaderDataKind::Vector))
              {
                switch(ShaderDataTypeGetScalarType(dataType).type)
                {
                SCALAR_SOP(float, FMul);
                SCALAR_SOP(uint, IMul);
                SCALAR_SOP(int, IMul);
                default:
                  throw Exception("unsupported SPIR-V shader data scalar type for multiplication");
                }
                break;
              }

              spv::Op op;
              // vector-scalar
              if(kind0 == ShaderDataKind::Vector && kind1 == ShaderDataKind::Scalar)
                op = spv::Op::OpVectorTimesScalar;
              // matrix-scalar
              else if(kind0 == ShaderDataKind::Matrix && kind1 == ShaderDataKind::Scalar)
                op = spv::Op::OpMatrixTimesScalar;
              // matrix-vector
              else if(kind0 == ShaderDataKind::Matrix && kind1 == ShaderDataKind::Vector)
                op = spv::Op::OpMatrixTimesVector;
              // vector-matrix
              else if(kind0 == ShaderDataKind::Vector && kind1 == ShaderDataKind::Matrix)
                op = spv::Op::OpVectorTimesMatrix;
              // matrix-matrix
              else if(kind0 == ShaderDataKind::Matrix && kind1 == ShaderDataKind::Matrix)
                op = spv::Op::OpMatrixTimesMatrix;
              else
                throw Exception("unsupported SPIR-V shader data kind combination for multiplication");

              emitSimpleOp(op);
            }
            break;

          OP_SCALAR(Divide,
            SCALAR_SOP(float, FDiv);
            SCALAR_SOP(uint, UDiv);
            SCALAR_SOP(int, SDiv);
          );
          OP_SCALAR(DPdx,
            SCALAR_SOP(float, DPdx);
          );
          OP_SCALAR(DPdy,
            SCALAR_SOP(float, DPdy);
          );

#undef OP_SCALAR
#undef SCALAR_SOP

// helper macros for GLSL instructions
#define GLSL_OP_INST(op, inst) \
          case ShaderOperationType::op: \
            resultId = EmitGLSLInst(function.code, GLSLstd450##inst, typeResultId, argsResultIds, argsCount); \
            break
#define GLSL_OP_SCALAR(op, body) \
          case ShaderOperationType::op: \
            switch(ShaderDataTypeGetScalarType(dataType).type) \
            { \
            body \
            default: \
              throw Exception("unsupported SPIR-V shader data scalar type for operation " #op); \
            } \
            break
#define GLSL_SCALAR_INST(scalarType, inst) \
            case ShaderDataScalarType::Type::_##scalarType: \
              resultId = EmitGLSLInst(function.code, GLSLstd450##inst, typeResultId, argsResultIds, argsCount); \
              break

          GLSL_OP_SCALAR(Abs,
            GLSL_SCALAR_INST(float, FAbs);
            GLSL_SCALAR_INST(int, SAbs);
          );
          GLSL_OP_INST(Floor, Floor);
          GLSL_OP_INST(Ceil, Ceil);
          GLSL_OP_INST(Fract, Fract);
          GLSL_OP_INST(Sqrt, Sqrt);
          GLSL_OP_INST(InverseSqrt, InverseSqrt);
          GLSL_OP_INST(Sin, Sin);
          GLSL_OP_INST(Cos, Cos);
          GLSL_OP_INST(Tan, Tan);
          GLSL_OP_INST(Asin, Asin);
          GLSL_OP_INST(Acos, Acos);
          GLSL_OP_INST(Atan, Atan);
          GLSL_OP_INST(Pow, Pow);
          GLSL_OP_INST(Exp, Exp);
          GLSL_OP_INST(Log, Log);
          GLSL_OP_INST(Exp2, Exp2);
          GLSL_OP_INST(Log2, Log2);
          GLSL_OP_SCALAR(Min,
            GLSL_SCALAR_INST(float, FMin);
            GLSL_SCALAR_INST(uint, UMin);
            GLSL_SCALAR_INST(int, SMin);
          );
          GLSL_OP_SCALAR(Max,
            GLSL_SCALAR_INST(float, FMax);
            GLSL_SCALAR_INST(uint, UMax);
            GLSL_SCALAR_INST(int, SMax);
          );
          GLSL_OP_SCALAR(Clamp,
            GLSL_SCALAR_INST(float, FClamp);
            GLSL_SCALAR_INST(uint, UClamp);
            GLSL_SCALAR_INST(int, SClamp);
          );
          GLSL_OP_SCALAR(Mix,
            GLSL_SCALAR_INST(float, FMix);
          );
          case ShaderOperationType::Dot:
            emitSimpleOp(spv::Op::OpDot);
            break;
          GLSL_OP_INST(Length, Length);
          GLSL_OP_INST(Distance, Distance);
          GLSL_OP_INST(Cross, Cross);
          GLSL_OP_INST(Normalize, Normalize);

#undef GLSL_OP_INST
#undef GLSL_OP_SCALAR
#undef GLSL_SCALAR_INST

          default:
            throw Exception("unknown SPIR-V shader operation type");
          }
        }
        break;
      case ShaderExpressionType::Read:
        {
          ShaderReadNode* readNode = static_cast<ShaderReadNode*>(node);
          ResultId variableResultId = TraverseVariable(function, readNode->node.get(), false).first;
          EmitOp(function.code, spv::Op::OpLoad, [&]()
          {
            Emit(function.code, typeResultId);
            resultId = EmitResultId(function.code);
            Emit(function.code, variableResultId);
          });
        }
        break;
      case ShaderExpressionType::Sample:
        {
          ShaderSampleNode* sampleNode = static_cast<ShaderSampleNode*>(node);
          ShaderSampledImageNode* sampledImageNode = sampleNode->sampledImageNode.get();
          ResultId sampledImageVariableResultId = TraverseSampledImage(sampledImageNode);
          ShaderDataType const& sampleDataType = sampledImageNode->GetDataType();
          ResultId typeResultId = TraverseType(sampleDataType);
          ResultId sampledImageTypeResultId = TraverseSampledImageType(sampleDataType, sampledImageNode->GetDimensionality());
          ShaderDataScalarType const& sampleComponentDataType = ShaderDataTypeGetScalarType(sampleDataType);
          ResultId sampleTypeResultId = TraverseType(ShaderDataVectorType(sampleComponentDataType, 4, sampleComponentDataType.GetSize() * 4));
          ResultId coordsResultId = TraverseExpression(function, sampleNode->coordsNode.get());
          ResultId sampledImageResultId;
          // resolve variable
          EmitOp(function.code, spv::Op::OpLoad, [&]()
          {
            Emit(function.code, sampledImageTypeResultId);
            sampledImageResultId = EmitResultId(function.code);
            Emit(function.code, sampledImageVariableResultId);
          });
          // sample image
          ResultId sampleResultId;
          EmitOp(function.code, spv::Op::OpImageSampleImplicitLod, [&]()
          {
            Emit(function.code, sampleTypeResultId);
            sampleResultId = EmitResultId(function.code);
            Emit(function.code, sampledImageResultId);
            Emit(function.code, coordsResultId);
            Emit(function.code, 0);
          });
          // sample is always 4-component; reduce size if required
          switch(sampleDataType.GetKind())
          {
          case ShaderDataKind::Scalar:
            EmitOp(function.code, spv::Op::OpCompositeExtract, [&]()
            {
              Emit(function.code, typeResultId);
              resultId = EmitResultId(function.code);
              Emit(function.code, sampleResultId);
              Emit(function.code, 0);
            });
            break;
          case ShaderDataKind::Vector:
            {
              ShaderDataVectorType const& sampleVectorDataType = static_cast<ShaderDataVectorType const&>(sampleDataType);
              if(sampleVectorDataType.n == 4)
              {
                resultId = sampleResultId;
              }
              else
              {
                EmitOp(function.code, spv::Op::OpVectorShuffle, [&]()
                {
                  Emit(function.code, typeResultId);
                  resultId = EmitResultId(function.code);
                  Emit(function.code, sampleResultId);
                  Emit(function.code, sampleResultId);
                  for(uint32_t i = 0; i < sampleVectorDataType.n; ++i)
                    Emit(function.code, i);
                });
              }
            }
            break;
          default:
            throw Exception("unsupported SPIR-V shader sample type");
          }

          _bindings[sampledImageNode->slotSet][sampledImageNode->slot].stageFlags |= (uint32_t)function.stage;
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
          ResultId variableResultId = TraverseVariable(function, writeNode->variableNode.get(), true).first;
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

    std::pair<ResultId, spv::StorageClass> TraverseVariable(Function& function, ShaderVariableNode* node, bool write)
    {
      spv::StorageClass storageClass = write ? spv::StorageClass::Output : spv::StorageClass::Input;
      std::optional<uint32_t> location;
      std::optional<spv::BuiltIn> builtin;
      std::optional<std::tuple<uint32_t, uint32_t, SpirvDescriptorType>> descriptor;
      std::optional<std::tuple<ResultId, ResultId>> accessChain;
      bool interfaceVariable = false;

      switch(node->GetVariableType())
      {
      case ShaderVariableType::Buffer:
        {
          ShaderBufferVariableNode* bufferNode = static_cast<ShaderBufferVariableNode*>(node);

          SpirvDescriptorType descriptorType = SpirvDescriptorType::Unused;
          switch(bufferNode->bufferType)
          {
          case ShaderBufferType::Uniform:
            descriptorType = SpirvDescriptorType::UniformBuffer;
            storageClass = spv::StorageClass::Uniform;
            break;
          case ShaderBufferType::Storage:
            descriptorType = SpirvDescriptorType::StorageBuffer;
            storageClass = spv::StorageClass::StorageBuffer;
            _extensions.insert("SPV_KHR_storage_buffer_storage_class");
            break;
          default:
            throw Exception("unsupported SPIR-V buffer type");
          }

          descriptor = { bufferNode->slotSet, bufferNode->slot, descriptorType };
        }
        break;
      case ShaderVariableType::StructMember:
        {
          ShaderStructMemberVariableNode* memberNode = static_cast<ShaderStructMemberVariableNode*>(node);
          auto structVariable = TraverseVariable(function, memberNode->structNode.get(), write);
          storageClass = structVariable.second;
          ResultId indexResultId = TraverseConst(memberNode->index);
          accessChain = { structVariable.first, indexResultId };
        }
        break;
      case ShaderVariableType::ArrayMember:
        {
          ShaderArrayMemberVariableNode* memberNode = static_cast<ShaderArrayMemberVariableNode*>(node);
          auto arrayVariable = TraverseVariable(function, memberNode->arrayNode.get(), write);
          storageClass = arrayVariable.second;
          ResultId indexResultId = TraverseExpression(function, memberNode->indexNode.get());
          accessChain = { arrayVariable.first, indexResultId };
        }
        break;
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
        interfaceVariable = true;
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
        interfaceVariable = true;
        break;
      case ShaderVariableType::TessLevelInner:
        builtin = spv::BuiltIn::TessLevelInner;
        interfaceVariable = true;
        break;
      case ShaderVariableType::TessLevelOuter:
        builtin = spv::BuiltIn::TessLevelOuter;
        interfaceVariable = true;
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
        interfaceVariable = true;
        break;
      default:
        throw Exception("unknown SPIR-V shader variable type");
      }

      auto it = function.variablesResultIds.find({ node, storageClass });
      if(it != function.variablesResultIds.end()) return { it->second, storageClass };
      it = function.variablesResultIds.insert({ { node, storageClass }, 0 }).first;

      ShaderDataType const& dataType = node->GetDataType();
      ResultId typeResultId = TraversePointerType(TraverseType(dataType), storageClass);
      ResultId resultId;

      if(accessChain.has_value())
      {
        auto [structResultId, indexResultId] = accessChain.value();
        EmitOp(function.code, spv::Op::OpAccessChain, [&, structResultId = structResultId, indexResultId = indexResultId]()
        {
          Emit(function.code, typeResultId);
          resultId = EmitResultId(function.code);
          Emit(function.code, structResultId);
          Emit(function.code, indexResultId);
        });
      }
      else
      {
        EmitOp(_codeDecls, spv::Op::OpVariable, [&]()
        {
          Emit(_codeDecls, typeResultId);
          resultId = EmitResultId(_codeDecls);
          Emit(_codeDecls, storageClass);
        });
      }

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
      else if(location.has_value())
      {
        EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&]()
        {
          Emit(_codeAnnotations, resultId);
          Emit(_codeAnnotations, spv::Decoration::Location);
          Emit(_codeAnnotations, location.value());
        });
      }

      if(descriptor.has_value())
      {
        auto [slotSet, slot, descriptorType] = descriptor.value();

        EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&, slotSet = slotSet]()
        {
          Emit(_codeAnnotations, resultId);
          Emit(_codeAnnotations, spv::Decoration::DescriptorSet);
          Emit(_codeAnnotations, slotSet);
        });
        EmitOp(_codeAnnotations, spv::Op::OpDecorate, [&, slot = slot]()
        {
          Emit(_codeAnnotations, resultId);
          Emit(_codeAnnotations, spv::Decoration::Binding);
          Emit(_codeAnnotations, slot);
        });

        SpirvDescriptorSetLayoutBinding& binding = _bindings[slotSet][slot];
        if(binding.descriptorType != descriptorType)
        {
          if(binding.descriptorType != SpirvDescriptorType::Unused)
            throw Exception("conflicting SPIR-V descriptor");
          binding.descriptorType = descriptorType;
          binding.descriptorCount = std::max<uint32_t>(binding.descriptorCount, 1);
        }
        binding.stageFlags |= (uint32_t)function.stage;
      }

      if(interfaceVariable)
      {
        switch(storageClass)
        {
        case spv::StorageClass::Input:
        case spv::StorageClass::Output:
          function.interfaceVariablesResultIds.insert(resultId);
          break;
        default:
          break;
        }
      }

      return { resultId, storageClass };
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
          // SPIR-V needs vector type of column
          std::unique_ptr<ShaderDataType const> columnTypePtr = std::make_unique<ShaderDataVectorType>(
            dataMatrixType.baseType,
            dataMatrixType.n,
            // does not account for alignment, but column's size is not used anywhere
            dataMatrixType.n * dataMatrixType.baseType.GetSize()
          );
          ShaderDataType const& columnType = *columnTypePtr;
          _tempTypes.push_back(std::move(columnTypePtr));
          resultId = TraverseCompositeType(spv::Op::OpTypeMatrix, columnType, dataMatrixType.m);
        }
        break;
      case ShaderDataKind::Array:
        {
          ShaderDataArrayType const& dataArrayType = static_cast<ShaderDataArrayType const&>(dataType);
          ResultId baseTypeResultId = TraverseType(dataArrayType.baseType);
          ResultId lengthResultId = TraverseConst(dataArrayType.n);
          EmitOp(_codeDecls, spv::Op::OpTypeArray, [&]()
          {
            resultId = EmitResultId(_codeDecls);
            Emit(_codeDecls, baseTypeResultId);
            Emit(_codeDecls, lengthResultId);
          });
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
    ResultId TraversePointerType(ResultId typeResultId, spv::StorageClass storageClass)
    {
      auto it = _pointerTypeResultIds.find({ typeResultId, storageClass });
      if(it != _pointerTypeResultIds.end()) return it->second;
      it = _pointerTypeResultIds.insert({ { typeResultId, storageClass }, 0 }).first;

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

    ResultId TraverseSampledImage(ShaderSampledImageNode* node)
    {
      auto it = _sampledImageResultIds.find(node);
      if(it != _sampledImageResultIds.end()) return it->second;

      ResultId typeResultId = TraversePointerType(TraverseSampledImageType(node->GetDataType(), node->GetDimensionality()), spv::StorageClass::UniformConstant);

      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpVariable, [&]()
      {
        Emit(_codeDecls, typeResultId);
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, spv::StorageClass::UniformConstant);
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

      _sampledImageResultIds.insert({ node, resultId });

      SpirvDescriptorSetLayoutBinding& binding = _bindings[node->slotSet][node->slot];
      if(binding.descriptorType != SpirvDescriptorType::SampledImage)
      {
        if(binding.descriptorType != SpirvDescriptorType::Unused)
          throw Exception("conflicting SPIR-V descriptor for sampled image");
        binding.descriptorType = SpirvDescriptorType::SampledImage;
        binding.descriptorCount = std::max<uint32_t>(binding.descriptorCount, 1);
      }

      return resultId;
    }

    ResultId TraverseSampledImageType(ShaderDataType const& dataType, size_t dimensionality)
    {
      ResultId imageTypeResultId = TraverseImageType(dataType, dimensionality);

      auto it = _sampledImageTypeResultIds.find(imageTypeResultId);
      if(it != _sampledImageTypeResultIds.end()) return it->second;

      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpTypeSampledImage, [&]()
      {
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, imageTypeResultId);
      });

      _sampledImageTypeResultIds.insert({ imageTypeResultId, resultId });

      return resultId;
    }

    ResultId TraverseImageType(ShaderDataType const& dataType, size_t dimensionality)
    {
      ResultId sampledTypeResultId = TraverseType(ShaderDataTypeGetScalarType(dataType));

      auto it = _imageTypeResultIds.find({ sampledTypeResultId, dimensionality });
      if(it != _imageTypeResultIds.end()) return it->second;

      ResultId resultId;
      EmitOp(_codeDecls, spv::Op::OpTypeImage, [&]()
      {
        resultId = EmitResultId(_codeDecls);
        Emit(_codeDecls, sampledTypeResultId);
        spv::Dim dim;
        switch(dimensionality)
        {
        case 1: dim = spv::Dim::Dim1D; break;
        case 2: dim = spv::Dim::Dim2D; break;
        case 3: dim = spv::Dim::Dim3D; break;
        default: throw Exception("unsupported image dimensionality");
        }
        Emit(_codeDecls, dim);
        Emit(_codeDecls, 0); // depth
        Emit(_codeDecls, 0); // arrayed
        Emit(_codeDecls, 0); // MS
        Emit(_codeDecls, 1); // sampled
        Emit(_codeDecls, spv::ImageFormat::Unknown);
      });

      _imageTypeResultIds.insert({ { sampledTypeResultId, dimensionality }, resultId });

      return resultId;
    }

    ResultId EmitGLSLInst(SpirvCode& code, GLSLstd450 inst, ResultId typeResultId, ResultId* const argsResultIds, size_t argsCount)
    {
      ResultId resultId;
      EmitOp(code, spv::Op::OpExtInst, [&]()
      {
        Emit(code, typeResultId);
        resultId = EmitResultId(code);
        Emit(code, _glslInstSetResultId);
        Emit(code, (uint32_t)inst);
        for(size_t i = 0; i < argsCount; ++i)
          Emit(code, argsResultIds[i]);
      });
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
    };

    // comparator for C strings
    struct CStringComparator
    {
      bool operator()(char const* a, char const* b) const
      {
        return strcmp(a, b) < 0;
      }
    };

    SpirvCode _codeAnnotations; // annotations
    SpirvCode _codeDecls; // types, constants, global variables
    SpirvCode _codeModule; // module code
    std::map<std::string, Function> _functions; // entry points
    std::map<ShaderDataType const*, ResultId, ShaderDataTypePtrComparator> _typeResultIds;
    std::vector<std::unique_ptr<ShaderDataType const>> _tempTypes; // temporary created types, stored on heap for references to stay valid
    std::map<std::pair<ResultId, spv::StorageClass>, ResultId> _pointerTypeResultIds;
    std::map<float, ResultId> _floatConstResultIds;
    std::map<uint32_t, ResultId> _uintConstResultIds;
    std::map<int32_t, ResultId> _intConstResultIds;
    std::map<bool, ResultId> _boolConstResultIds; // yeah, stupid
    std::map<ShaderBufferVariableNode*, ResultId> _structBufferResultIds;
    std::map<ShaderSampledImageNode*, ResultId> _sampledImageResultIds;
    std::map<ResultId, ResultId> _sampledImageTypeResultIds;
    std::map<std::pair<ResultId, size_t>, ResultId> _imageTypeResultIds;

    std::set<spv::Capability> _capabilities =
    {
      spv::Capability::Matrix,
    };
    std::set<char const*, CStringComparator> _extensions;
    uint32_t _tessellationOutputVertices = 0;
    ivec3 _computeSize;
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
    if(roots.tessellationControl || roots.tessellationEvaluation)
    {
      if(!(roots.tessellationControl && roots.tessellationEvaluation))
        throw Exception("Incomplete tessellation stages in SPIR-V shader");
      compiler.SetTessellationOutputVertices(roots.tessellationOutputVertices);
      compiler.TraverseEntryPoint("mainTessellationControl", SpirvStageFlag::TessellationControl, spv::ExecutionModel::TessellationControl, roots.tessellationControl.get());
      compiler.TraverseEntryPoint("mainTessellationEvaluation", SpirvStageFlag::TessellationEvaluation, spv::ExecutionModel::TessellationEvaluation, roots.tessellationEvaluation.get());
    }
    if(roots.fragment)
      compiler.TraverseEntryPoint("mainFragment", SpirvStageFlag::Fragment, spv::ExecutionModel::Fragment, roots.fragment.get());
    if(roots.compute)
    {
      compiler.SetComputeSize(roots.computeSize);
      compiler.TraverseEntryPoint("mainCompute", SpirvStageFlag::Compute, spv::ExecutionModel::GLCompute, roots.compute.get());
    }

    return compiler.Finalize();
  }
}
