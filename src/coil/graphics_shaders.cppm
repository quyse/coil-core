module;

#include <array>
#include <concepts>
#include <memory>
#include <type_traits>
#include <vector>

export module coil.core.graphics.shaders;

import coil.core.base;
import coil.core.graphics.format;
import coil.core.math;
import coil.core.util;

export namespace Coil
{
  enum class ShaderDataKind : uint8_t
  {
    Scalar,
    Vector,
    Matrix,
    Array,
    Struct,
  };

  struct ShaderDataScalarType;

  struct ShaderDataType
  {
    ShaderDataType(ShaderDataType const&) = delete;
    ShaderDataType(ShaderDataType&&) = delete;
    ShaderDataType& operator=(ShaderDataType const&) = delete;
    ShaderDataType& operator=(ShaderDataType&&) = delete;

    virtual ~ShaderDataType() = default;

    virtual ShaderDataKind GetKind() const = 0;
    virtual uint32_t GetSize() const = 0;

  protected:
    ShaderDataType() = default;
  };

  struct ShaderDataScalarType final : public ShaderDataType
  {
    enum class Type : uint8_t
    {
      _float,
      _uint,
      _int,
      _bool,
    };

    ShaderDataScalarType(Type type, uint32_t size)
    : type(type), size(size) {}

    Type type;
    uint32_t size;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Scalar;
    }

    uint32_t GetSize() const override
    {
      return size;
    }
  };

  struct ShaderDataVectorType final : public ShaderDataType
  {
    ShaderDataVectorType(ShaderDataScalarType const& baseType, uint32_t n, uint32_t size)
    : baseType(baseType), n(n), size(size) {}

    ShaderDataScalarType const& baseType;
    uint32_t n, size;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Vector;
    }

    uint32_t GetSize() const override
    {
      return size;
    }
  };

  struct ShaderDataMatrixType final : public ShaderDataType
  {
    ShaderDataMatrixType(ShaderDataScalarType const& baseType, uint32_t n, uint32_t m, uint32_t rowStride, uint32_t size)
    : baseType(baseType), n(n), m(m), rowStride(rowStride), size(size) {}

    ShaderDataScalarType const& baseType;
    uint32_t n, m, rowStride, size;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Matrix;
    }

    uint32_t GetSize() const override
    {
      return size;
    }
  };

  struct ShaderDataArrayType final : public ShaderDataType
  {
    ShaderDataArrayType(ShaderDataType const& baseType, uint32_t n)
    : baseType(baseType), n(n) {}

    ShaderDataType const& baseType;
    uint32_t n;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Array;
    }

    uint32_t GetSize() const override
    {
      return n * baseType.GetSize();
    }
  };

  struct ShaderDataStructType final : public ShaderDataType
  {
    using Member = std::pair<uint32_t, ShaderDataType const&>;

    ShaderDataStructType(std::vector<Member>&& members, uint32_t size)
    : members(std::move(members)), size(size) {}

    std::vector<Member> members;
    uint32_t size;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Struct;
    }

    uint32_t GetSize() const override
    {
      return size;
    }
  };

  // get scalar type of scalar, vector, matrix type
  ShaderDataScalarType const& ShaderDataTypeGetScalarType(ShaderDataType const& dataType)
  {
    switch(dataType.GetKind())
    {
    case ShaderDataKind::Scalar:
      return static_cast<ShaderDataScalarType const&>(dataType);
    case ShaderDataKind::Vector:
      return static_cast<ShaderDataVectorType const&>(dataType).baseType;
    case ShaderDataKind::Matrix:
      return static_cast<ShaderDataMatrixType const&>(dataType).baseType;
    default:
      throw Exception("unsupported SPIR-V shader data kind for getting scalar data type");
    }
  }

  // comparison operator supports all kinds of types
  bool operator<(ShaderDataType const& a, ShaderDataType const& b)
  {
    auto ak = a.GetKind();
    auto bk = b.GetKind();
    if(ak < bk) return true;
    if(bk < ak) return false;
    switch(ak)
    {
    case ShaderDataKind::Scalar:
      {
        auto const& as = static_cast<ShaderDataScalarType const&>(a);
        auto const& bs = static_cast<ShaderDataScalarType const&>(b);
        return as.type < bs.type;
      }
    case ShaderDataKind::Vector:
      {
        auto const& av = static_cast<ShaderDataVectorType const&>(a);
        auto const& bv = static_cast<ShaderDataVectorType const&>(b);
        if(av.baseType < bv.baseType) return true;
        if(bv.baseType < av.baseType) return false;
        return av.n < bv.n;
      }
    case ShaderDataKind::Matrix:
      {
        auto const& am = static_cast<ShaderDataMatrixType const&>(a);
        auto const& bm = static_cast<ShaderDataMatrixType const&>(b);
        if(am.baseType < bm.baseType) return true;
        if(bm.baseType < am.baseType) return false;
        if(am.n < bm.n) return true;
        if(bm.n < am.n) return false;
        return am.m < bm.m;
      }
    case ShaderDataKind::Array:
      {
        auto const& aa = static_cast<ShaderDataArrayType const&>(a);
        auto const& ba = static_cast<ShaderDataArrayType const&>(b);
        if(aa.baseType < ba.baseType) return true;
        if(ba.baseType < aa.baseType) return false;
        return aa.n < ba.n;
      }
    case ShaderDataKind::Struct:
      {
        auto const& as = static_cast<ShaderDataStructType const&>(a);
        auto const& bs = static_cast<ShaderDataStructType const&>(b);
        return as.members < bs.members;
      }
    }
  }

  template <typename T>
  struct ShaderDataTypeOfHelper;

  template <>
  struct ShaderDataTypeOfHelper<float>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_float, sizeof(float));
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<uint32_t>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_uint, sizeof(uint32_t));
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<int32_t>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_int, sizeof(int32_t));
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<bool>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_bool, 1);
      return t;
    }
  };

  template <typename T, uint32_t n>
  struct ShaderDataTypeOfHelper<xvec<T, n>>
  {
    static ShaderDataVectorType const& GetType()
    {
      static ShaderDataVectorType const t(ShaderDataTypeOfHelper<T>::GetType(), n, sizeof(xvec<T, n>));
      return t;
    }
  };
  // quaternions are simply vectors
  template <typename T>
  struct ShaderDataTypeOfHelper<xquat<T>>
  {
    static ShaderDataVectorType const& GetType()
    {
      return ShaderDataTypeOfHelper<xvec<T, 4>>::GetType();
    }
  };

  template <typename T, uint32_t n, uint32_t m>
  struct ShaderDataTypeOfHelper<xmat<T, n, m>>
  {
    static ShaderDataMatrixType const& GetType()
    {
      static ShaderDataMatrixType const t(ShaderDataTypeOfHelper<T>::GetType(), n, m, sizeof(typename xmat<T, n, m>::Row), sizeof(xmat<T, n, m>));
      return t;
    }
  };

  template <typename T, uint32_t n>
  struct ShaderDataTypeOfHelper<T[n]>
  {
    static ShaderDataType const& GetType()
    {
      static ShaderDataArrayType const t(ShaderDataTypeOfHelper<T>::GetType(), n);
      return t;
    }
  };

  template <typename T>
  ShaderDataType const& ShaderDataTypeOf()
  {
    return ShaderDataTypeOfHelper<T>::GetType();
  }

  // node type in shader
  enum class ShaderNodeType
  {
    // expressions have value type
    Expression,
    // statements don't have value type
    Statement,
    // variables have value type and can be written to
    Variable,
    // sampled image
    SampledImage,
  };

  enum class ShaderExpressionType
  {
    // general operation which accepts expressions as arguments
    Operation,
    // reads variable
    Read,
    // samples sampled image
    Sample,
  };

  enum class ShaderOperationType
  {
    Const,
    Cast,
    Construct,
    Swizzle,
    Index,
    Negate,
    Add,
    Subtract,
    Multiply,
    Divide,
    DPdx,
    DPdy,
    Abs,
    Floor,
    Ceil,
    Fract,
    Sqrt,
    InverseSqrt,
    Sin,
    Cos,
    Tan,
    Asin,
    Acos,
    Atan,
    Pow,
    Exp,
    Log,
    Exp2,
    Log2,
    Min,
    Max,
    Clamp,
    Mix,
    Dot,
    Length,
    Distance,
    Cross,
    Normalize,
  };

  enum class ShaderStatementType
  {
    Sequence,
    Write,
  };

  enum class ShaderVariableType
  {
    Buffer,
    StructMember,
    ArrayMember,
    Attribute,
    Interpolant,
    TessLevelInner,
    TessLevelOuter,
    Fragment,
  };

  struct ShaderNode
  {
    virtual ~ShaderNode() = default;

    virtual ShaderNodeType GetNodeType() const = 0;
  };

  struct ShaderExpressionNode : public ShaderNode
  {
    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Expression;
    }

    virtual ShaderExpressionType GetExpressionType() const = 0;
    virtual ShaderDataType const& GetDataType() const = 0;
  };

  struct ShaderOperationNode : public ShaderExpressionNode
  {
    ShaderExpressionType GetExpressionType() const override
    {
      return ShaderExpressionType::Operation;
    }

    virtual ShaderOperationType GetOperationType() const = 0;
    virtual size_t GetArgsCount() const = 0;
    virtual std::shared_ptr<ShaderExpressionNode> GetArg(size_t i) const = 0;
  };

  // operation expression
  template <typename T, ShaderOperationType opType, size_t n>
  struct ShaderOperationNodeImpl : public ShaderOperationNode
  {
    ShaderOperationNodeImpl(std::array<std::shared_ptr<ShaderExpressionNode>, n>&& args)
    : args(std::move(args)) {}

    ShaderOperationType GetOperationType() const override
    {
      return opType;
    }

    ShaderDataType const& GetDataType() const override
    {
      return ShaderDataTypeOf<T>();
    }

    size_t GetArgsCount() const override
    {
      return n;
    }

    std::shared_ptr<ShaderExpressionNode> GetArg(size_t i) const override
    {
      return args[i];
    }

    std::array<std::shared_ptr<ShaderExpressionNode>, n> args;
  };

  template <typename T>
  struct ShaderOperationConstNode : public ShaderOperationNodeImpl<T, ShaderOperationType::Const, 0>
  {
    ShaderOperationConstNode(T&& value)
    : ShaderOperationNodeImpl<T, ShaderOperationType::Const, 0>({}), value(std::move(value)) {}

    T value;
  };

  struct ShaderOperationSwizzleNode : public ShaderOperationNode
  {
    ShaderOperationSwizzleNode(ShaderDataType const& dataType, std::shared_ptr<ShaderExpressionNode> node, char const* swizzle)
    : dataType(dataType), node(std::move(node)), swizzle(swizzle) {}

    ShaderDataType const& dataType;
    std::shared_ptr<ShaderExpressionNode> node;
    char const* swizzle;

    ShaderOperationType GetOperationType() const override
    {
      return ShaderOperationType::Swizzle;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }

    size_t GetArgsCount() const override
    {
      return 1;
    }

    std::shared_ptr<ShaderExpressionNode> GetArg(size_t i) const override
    {
      return node;
    }
  };

  template <typename T>
  struct ShaderExpression
  {
    // init with expression
    template <typename Node>
    ShaderExpression(std::shared_ptr<Node>&& node)
    : node(std::move(node)) {}

    ShaderExpression() = delete;
    ShaderExpression(ShaderExpression const&) = default;

    // constant initialization
    template <typename... Args>
    explicit ShaderExpression(Args&& ...args)
    : node(std::make_shared<ShaderOperationConstNode<T>>(T(std::forward<Args>(args)...))) {}

    std::shared_ptr<ShaderExpressionNode> node;

    // cast
    template <typename TT>
    explicit operator ShaderExpression<TT>() const
    {
      return std::make_shared<ShaderOperationNodeImpl<TT, ShaderOperationType::Cast, 1>>(std::array { node });
    }

    // operations
    ShaderExpression operator-() const
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Negate, 1>>(std::array { node });
    }

    // mutations
    ShaderExpression& operator+=(ShaderExpression const& b)
    {
      return *this = *this + b;
    }
    ShaderExpression& operator-=(ShaderExpression const& b)
    {
      return *this = *this - b;
    }
    ShaderExpression& operator*=(ShaderExpression const& b)
    {
      return *this = *this * b;
    }
    ShaderExpression& operator/=(ShaderExpression const& b)
    {
      return *this = *this / b;
    }
  };

  template <ShaderOperationType op, typename R, typename... Args>
  ShaderExpression<R> ShaderOperation(ShaderExpression<Args> const&... args)
  {
    return std::make_shared<ShaderOperationNodeImpl<R, op, sizeof...(args)>>(std::array<std::shared_ptr<ShaderExpressionNode>, sizeof...(args)> { (args.node)... });
  }

  template <typename T>
  ShaderExpression<T> operator+(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
  {
    return ShaderOperation<ShaderOperationType::Add, T>(a, b);
  }
  template <typename T>
  ShaderExpression<T> operator-(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
  {
    return ShaderOperation<ShaderOperationType::Subtract, T>(a, b);
  }
  template <typename T>
  ShaderExpression<T> operator*(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, T>(a, b);
  }
  template <typename T>
  ShaderExpression<T> operator/(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
  {
    return ShaderOperation<ShaderOperationType::Divide, T>(a, b);
  }

  // vector-scalar multiplication
  template <typename T, size_t n>
  ShaderExpression<xvec<T, n>> operator*(ShaderExpression<xvec<T, n>> const& a, ShaderExpression<T> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, xvec<T, n>>(a, b);
  }
  // scalar-vector multiplication
  template <typename T, size_t n>
  ShaderExpression<xvec<T, n>> operator*(ShaderExpression<T> const& a, ShaderExpression<xvec<T, n>> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, xvec<T, n>>(a, b);
  }

  // matrix-matrix multiplication
  template <size_t n, size_t m, size_t k>
  ShaderExpression<xmat<float, n, m>> operator*(ShaderExpression<xmat<float, n, k>> const& a, ShaderExpression<xmat<float, k, m>> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, xmat<float, n, m>>(a, b);
  }
  // matrix-vector multiplication
  template <size_t n, size_t m>
  ShaderExpression<xvec<float, n>> operator*(ShaderExpression<xmat<float, n, m>> const& a, ShaderExpression<xvec<float, m>> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, xvec<float, n>>(a, b);
  }
  // vector-matrix multiplication
  template <size_t n, size_t m>
  ShaderExpression<xvec<float, m>> operator*(ShaderExpression<xvec<float, n>> const& a, ShaderExpression<xmat<float, n, m>> const& b)
  {
    return ShaderOperation<ShaderOperationType::Multiply, xvec<float, m>>(a, b);
  }

  // swizzle
  template <typename T, size_t n, size_t m>
  ShaderExpression<typename VectorTraits<xvec<T, m - 1>>::PossiblyScalar> swizzle(ShaderExpression<xvec<T, n>> const& a, char const (&swizzle)[m])
  {
    return std::make_shared<ShaderOperationSwizzleNode>(ShaderDataTypeOf<typename VectorTraits<xvec<T, m - 1>>::PossiblyScalar>(), a.node, swizzle);
  }
  template <typename T, size_t m>
  ShaderExpression<typename VectorTraits<xvec<T, m - 1>>::PossiblyScalar> swizzle(ShaderExpression<xquat<T>> const& a, char const (&swizzle)[m])
  {
    return std::make_shared<ShaderOperationSwizzleNode>(ShaderDataTypeOf<typename VectorTraits<xvec<T, m - 1>>::PossiblyScalar>(), a.node, swizzle);
  }

  struct ShaderStatementNode : public ShaderNode
  {
    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Statement;
    }

    virtual ShaderStatementType GetStatementType() const = 0;
  };

  struct ShaderStatementSequenceNode final : public ShaderStatementNode
  {
    ShaderStatementSequenceNode(std::shared_ptr<ShaderStatementNode> a, std::shared_ptr<ShaderStatementNode> b)
    : a(std::move(a)), b(std::move(b)) {}

    ShaderStatementType GetStatementType() const override
    {
      return ShaderStatementType::Sequence;
    }

    std::shared_ptr<ShaderStatementNode> a, b;
  };

  struct ShaderStatement
  {
    template <typename Node>
    ShaderStatement(std::shared_ptr<Node>&& node)
    : node(std::move(node)) {}

    std::shared_ptr<ShaderStatementNode> node;

    operator std::shared_ptr<ShaderStatementNode>() const
    {
      return node;
    }
  };

  // comma operator
  inline ShaderStatement operator,(ShaderStatement const& a, ShaderStatement const& b)
  {
    return std::make_shared<ShaderStatementSequenceNode>(a.node, b.node);
  }

  struct ShaderVariableNode : public ShaderNode
  {
    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Variable;
    }

    virtual ShaderVariableType GetVariableType() const = 0;
    virtual ShaderDataType const& GetDataType() const = 0;
  };

  // buffer type
  enum class ShaderBufferType
  {
    Uniform,
    Storage,
  };

  struct ShaderBufferVariableNode : public ShaderVariableNode
  {
    ShaderBufferVariableNode(ShaderDataType const& dataType, uint32_t slotSet, uint32_t slot, ShaderBufferType bufferType)
    : dataType(dataType), slotSet(slotSet), slot(slot), bufferType(bufferType) {}

    ShaderDataType const& dataType;
    uint32_t slotSet;
    uint32_t slot;
    ShaderBufferType bufferType;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Buffer;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  struct ShaderStructMemberVariableNode : public ShaderVariableNode
  {
    ShaderStructMemberVariableNode(std::shared_ptr<ShaderVariableNode> structNode, ShaderDataType const& dataType, uint32_t index)
    : structNode(std::move(structNode)), dataType(dataType), index(index) {}

    std::shared_ptr<ShaderVariableNode> structNode;
    ShaderDataType const& dataType;
    uint32_t index;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::StructMember;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  struct ShaderArrayMemberVariableNode : public ShaderVariableNode
  {
    ShaderArrayMemberVariableNode(std::shared_ptr<ShaderVariableNode> arrayNode, ShaderDataType const& dataType, std::shared_ptr<ShaderExpressionNode> indexNode)
    : arrayNode(std::move(arrayNode)), dataType(dataType), indexNode(indexNode) {}

    std::shared_ptr<ShaderVariableNode> arrayNode;
    ShaderDataType const& dataType;
    std::shared_ptr<ShaderExpressionNode> indexNode;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::ArrayMember;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  struct ShaderSampledImageNode : public ShaderNode
  {
    ShaderSampledImageNode(uint32_t slotSet, uint32_t slot)
    : slotSet(slotSet), slot(slot) {}

    uint32_t slotSet;
    uint32_t slot;

    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::SampledImage;
    }

    virtual ShaderDataType const& GetDataType() const = 0;
    virtual size_t GetDimensionality() const = 0;
  };

  template <typename T, size_t n>
  struct ShaderSampledImageNodeImpl : public ShaderSampledImageNode
  {
    ShaderSampledImageNodeImpl(uint32_t slotSet, uint32_t slot)
    : ShaderSampledImageNode(slotSet, slot) {}

    ShaderDataType const& GetDataType() const override
    {
      return ShaderDataTypeOf<T>();
    }

    size_t GetDimensionality() const override
    {
      return n;
    }
  };

  struct ShaderSampleNode : public ShaderExpressionNode
  {
    ShaderSampleNode(std::shared_ptr<ShaderSampledImageNode> sampledImageNode, std::shared_ptr<ShaderExpressionNode> coordsNode)
    : sampledImageNode(sampledImageNode), coordsNode(coordsNode) {}

    std::shared_ptr<ShaderSampledImageNode> sampledImageNode;
    std::shared_ptr<ShaderExpressionNode> coordsNode;

    ShaderExpressionType GetExpressionType() const override
    {
      return ShaderExpressionType::Sample;
    }

    ShaderDataType const& GetDataType() const override
    {
      return sampledImageNode->GetDataType();
    }
  };

  template <typename T, size_t n>
  struct ShaderSampledImage
  {
    ShaderSampledImage(uint32_t slotSet, uint32_t slot)
    : node(std::make_shared<ShaderSampledImageNodeImpl<T, n>>(slotSet, slot)) {}

    std::shared_ptr<ShaderSampledImageNodeImpl<T, n>> node;

    ShaderExpression<T> Sample(ShaderExpression<xvec<float, n>> const& coords) const
    {
      return std::make_shared<ShaderSampleNode>(node, coords.node);
    }
  };

  struct ShaderReadNode : public ShaderExpressionNode
  {
    ShaderReadNode(std::shared_ptr<ShaderVariableNode> node)
    : node(std::move(node)) {}

    std::shared_ptr<ShaderVariableNode> node;

    ShaderExpressionType GetExpressionType() const override
    {
      return ShaderExpressionType::Read;
    }
  };

  template <typename T>
  struct ShaderReadNodeImpl : public ShaderReadNode
  {
    ShaderReadNodeImpl(std::shared_ptr<ShaderVariableNode> node)
    : ShaderReadNode(std::move(node)) {}

    ShaderDataType const& GetDataType() const override
    {
      return ShaderDataTypeOf<T>();
    }
  };

  struct ShaderStatementWriteNode final : public ShaderStatementNode
  {
    ShaderStatementWriteNode(std::shared_ptr<ShaderVariableNode> variableNode, std::shared_ptr<ShaderExpressionNode> expressionNode)
    : variableNode(variableNode), expressionNode(expressionNode) {}

    ShaderStatementType GetStatementType() const override
    {
      return ShaderStatementType::Write;
    }

    std::shared_ptr<ShaderVariableNode> variableNode;
    std::shared_ptr<ShaderExpressionNode> expressionNode;
  };

  enum class ShaderAttributeBuiltin
  {
    None,
    VertexIndex,
    InstanceIndex,
  };

  struct ShaderVariableAttributeNode final : public ShaderVariableNode
  {
    ShaderVariableAttributeNode(ShaderDataType const& dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableAttributeNode(ShaderDataType const& dataType, ShaderAttributeBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType const& dataType;
    uint32_t location = -1;
    ShaderAttributeBuiltin builtin = ShaderAttributeBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Attribute;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  enum class ShaderInterpolantBuiltin
  {
    None,
    Position,
    PointSize,
    FragCoord,
  };

  struct ShaderVariableInterpolantNode final : public ShaderVariableNode
  {
    ShaderVariableInterpolantNode(ShaderDataType const& dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableInterpolantNode(ShaderDataType const& dataType, ShaderInterpolantBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType const& dataType;
    uint32_t location = -1;
    ShaderInterpolantBuiltin builtin = ShaderInterpolantBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Interpolant;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  struct ShaderVariableTessLevelInnerNode final : public ShaderVariableNode
  {
    ShaderVariableTessLevelInnerNode() = default;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::TessLevelInner;
    }

    ShaderDataType const& GetDataType() const override
    {
      return ShaderDataTypeOf<float[2]>();
    }
  };

  struct ShaderVariableTessLevelOuterNode final : public ShaderVariableNode
  {
    ShaderVariableTessLevelOuterNode() = default;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::TessLevelOuter;
    }

    ShaderDataType const& GetDataType() const override
    {
      return ShaderDataTypeOf<float[4]>();
    }
  };

  enum class ShaderFragmentBuiltin
  {
    None,
    FragDepth,
  };

  struct ShaderVariableFragmentNode final : public ShaderVariableNode
  {
    ShaderVariableFragmentNode(ShaderDataType const& dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableFragmentNode(ShaderDataType const& dataType, ShaderFragmentBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType const& dataType;
    uint32_t location = -1;
    ShaderFragmentBuiltin builtin = ShaderFragmentBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Fragment;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
    }
  };

  // helper template for discovering struct members
  template <std::vector<ShaderDataStructType::Member>& members>
  struct ShaderDataStructTypeOfHelper
  {
    template <typename M>
    struct Member
    {
      Member()
      {
        members.push_back({ (size_t)this, ShaderDataTypeOf<M>() });
      }

      // include value itself to keep size/alignment
      M m;
    };

    static ShaderDataStructType Finalize(void const* object, size_t size)
    {
      // calculate member offsets
      for(size_t i = 0; i < members.size(); ++i)
        members[i].first -= (size_t)object;

      return ShaderDataStructType(std::move(members), size);
    };
  };

  // mutable struct members vector, initialized once by ShaderDataStructTypeOf
  // it'd be better to be static local variable, but that hits a linking error
  template <template <template <typename> typename> typename T>
  std::vector<ShaderDataStructType::Member> shaderDataStructTypeOfMembers;

  // get type of struct
  template <template <template <typename> typename> typename T>
  ShaderDataStructType const& ShaderDataStructTypeOf()
  {
    // register members
    static T<ShaderDataStructTypeOfHelper<shaderDataStructTypeOfMembers<T>>::template Member> const object;
    // finalize type
    static ShaderDataStructType const type = ShaderDataStructTypeOfHelper<shaderDataStructTypeOfMembers<T>>::Finalize(&object, sizeof(object));

    return type;
  };


  // identity tranform for struct
  template <template <template <typename> typename> typename T>
  using ShaderDataIdentityStruct = T<std::type_identity_t>;


  template <typename T>
  struct ShaderVariable
  {
    template <typename Node>
    ShaderVariable(std::shared_ptr<Node>&& node)
    : node(std::move(node)) {}

    std::shared_ptr<ShaderVariableNode> node;

    ShaderExpression<T> operator*() const
    {
      return ShaderExpression<T>(std::make_shared<ShaderReadNodeImpl<T>>(node));
    }

    template <std::integral I>
    ShaderVariable<std::remove_extent_t<T>> operator[](ShaderExpression<I> indexExpression) const
    requires (std::is_array_v<T>)
    {
      return std::make_shared<ShaderArrayMemberVariableNode>(node, ShaderDataTypeOf<std::remove_extent_t<T>>(), indexExpression.node);
    }

    ShaderStatement Write(ShaderExpression<T> expression) const
    {
      return ShaderStatement(std::make_shared<ShaderStatementWriteNode>(node, expression.node));
    }
  };


  // struct establishing slot for struct buffer
  template <template <template <typename> typename> typename T, uint32_t slotSet, uint32_t slot, ShaderBufferType bufferType>
  struct ShaderDataStructBufferSlotInitializer
  {
    std::shared_ptr<ShaderBufferVariableNode> node =
      std::make_shared<ShaderBufferVariableNode>(ShaderDataStructTypeOf<T>(), slotSet, slot, bufferType);

    // counter for member initialization
    uint32_t nextMember = 0;
  };

  // initializer object for struct buffer slots
  // It would be nicer to make it local static variable of GetShaderDataStructBuffer,
  // as C++ >=17 allows to reference local variable in template argument,
  // but in practice it hits linker debug info bugs.
  // So, a template variable instead.
  template <template <template <typename> typename> typename T, uint32_t slotSet, uint32_t slot, ShaderBufferType bufferType>
  ShaderDataStructBufferSlotInitializer<T, slotSet, slot, bufferType> shaderDataStructBufferSlotInitializer;

  template <auto& initializer>
  struct ShaderDataStructBufferHelper
  {
    template <typename T>
    struct Member : public ShaderVariable<T>
    {
      Member()
      : ShaderVariable<T>(std::make_shared<ShaderStructMemberVariableNode>(
          initializer.node,
          ShaderDataTypeOf<T>(),
          initializer.nextMember++
        )) {}
    };
  };

  template <template <template <typename> typename> typename T, uint32_t slotSet, uint32_t slot, ShaderBufferType bufferType>
  auto const& GetShaderDataStructBuffer()
  {
    static T<ShaderDataStructBufferHelper<shaderDataStructBufferSlotInitializer<T, slotSet, slot, bufferType>>::template Member> const structBuffer;

    return structBuffer;
  }


  template <typename T>
  ShaderVariable<T> ShaderAttribute(uint32_t location)
  {
    return std::make_shared<ShaderVariableAttributeNode>(ShaderDataTypeOf<T>(), location);
  }
  template <typename T>
  ShaderVariable<T> ShaderAttribute(ShaderAttributeBuiltin builtin)
  {
    return std::make_shared<ShaderVariableAttributeNode>(ShaderDataTypeOf<T>(), builtin);
  }

  template <typename T>
  ShaderVariable<T> ShaderInterpolant(uint32_t location)
  {
    return std::make_shared<ShaderVariableInterpolantNode>(ShaderDataTypeOf<T>(), location);
  }
  template <typename T>
  ShaderVariable<T> ShaderInterpolant(ShaderInterpolantBuiltin builtin)
  {
    return std::make_shared<ShaderVariableInterpolantNode>(ShaderDataTypeOf<T>(), builtin);
  }

  inline auto ShaderInterpolantBuiltinPosition()
  {
    return ShaderInterpolant<vec4>(ShaderInterpolantBuiltin::Position);
  }

  inline ShaderVariable<float[2]> ShaderTessLevelInner()
  {
    return std::make_shared<ShaderVariableTessLevelInnerNode>();
  }
  inline ShaderVariable<float[4]> ShaderTessLevelOuter()
  {
    return std::make_shared<ShaderVariableTessLevelOuterNode>();
  }

  template <typename T>
  ShaderVariable<T> ShaderFragment(uint32_t location)
  {
    return std::make_shared<ShaderVariableFragmentNode>(ShaderDataTypeOf<T>(), location);
  }
  template <typename T>
  ShaderVariable<T> ShaderFragment(ShaderFragmentBuiltin builtin)
  {
    return std::make_shared<ShaderVariableFragmentNode>(ShaderDataTypeOf<T>(), builtin);
  }


  // vertex layout description
  class GraphicsVertexLayout
  {
  public:
    struct Slot
    {
      uint32_t stride;
      bool perInstance = false;
    };

    struct Attribute
    {
      uint32_t slot;
      uint32_t offset;
      VertexFormat format;
    };

    std::vector<Slot> slots;
    std::vector<Attribute> attributes;
  };

  template <template <template <typename> typename> typename... Structs>
  struct ShaderDataVertexStructLayoutInitializer
  {
    GraphicsVertexLayout layout;
    void const* pStructs[sizeof...(Structs)];
    uint32_t nextLocation = 0;
  };

  template <template <template <typename> typename> typename... Structs>
  ShaderDataVertexStructLayoutInitializer<Structs...> shaderDataVertexStructLayoutInitializer;

  template <auto& initializer>
  struct ShaderDataVertexStructLayoutHelper
  {
    template <template <template <typename> typename> typename Struct, uint32_t slot>
    struct SlotInitializer
    {
      SlotInitializer()
      : pStruct(initializer.pStructs[slot] = &s)
      {
        initializer.layout.slots.push_back(
        {
          .stride = sizeof(s),
        });
      }

      template <typename Field>
      struct InitializerMember
      {
        InitializerMember()
        {
          initializer.layout.attributes.push_back(
          {
            .slot = slot,
            .offset = (uint32_t)((uint8_t const*)this - (uint8_t const*)initializer.pStructs[slot]),
            .format = VertexFormatTraits<Field>::format,
          });
        }

        Field field;
      };

      void const* const pStruct;
      Struct<InitializerMember> const s;
    };

    template <typename Field>
    struct ValueMember : public ShaderVariable<typename VertexFormatTraits<Field>::Value>
    {
      ValueMember()
      : ShaderVariable<typename VertexFormatTraits<Field>::Value>(std::make_shared<ShaderVariableAttributeNode>(
          ShaderDataTypeOf<typename VertexFormatTraits<Field>::Value>(),
          initializer.nextLocation++
        )) {}
    };
  };

  template <typename Slots>
  struct GraphicsVertexStructLayout
  {
    GraphicsVertexStructLayout(GraphicsVertexLayout const& layout, Slots const& slots)
    : layout(layout), slots(slots) {}

    GraphicsVertexLayout const& layout;
    Slots const& slots;
  };

  template <template <template <typename> typename> typename... Structs>
  auto GetGraphicsVertexStructLayout()
  {
    static Tuple<
      Structs<
        ShaderDataVertexStructLayoutHelper<
          shaderDataVertexStructLayoutInitializer<Structs...>
        >::template ValueMember
      >...
    > const structSlots;

    return GraphicsVertexStructLayout([]<uint32_t... I>(std::integer_sequence<uint32_t, I...> seq) -> GraphicsVertexLayout const&
    {
      static Tuple<
        typename ShaderDataVertexStructLayoutHelper<
          shaderDataVertexStructLayoutInitializer<Structs...>
        >::template SlotInitializer<Structs, I>...
      > const structSlots;

      return shaderDataVertexStructLayoutInitializer<Structs...>.layout;
    }(std::make_integer_sequence<uint32_t, sizeof...(Structs)>()), structSlots);
  }


  // helper struct for types composing a vector
  template <typename T, typename... TT>
  struct ShaderVectorComposeHelper
  {
    // size of resulting vector
    static constexpr size_t N = (VectorTraits<T>::N + ... + VectorTraits<TT>::N);
    // scalar type of resulting vector
    using Scalar = typename VectorTraits<T>::Scalar;
    // is it ok to compose these types into vector
    static constexpr bool Ok =
      // all types should have same scalar type
      (std::same_as<Scalar, typename VectorTraits<TT>::Scalar> && ...) &&
      // size requirements
      N >= 2 && N <= 4;
    // result vector type
    using Result = xvec<Scalar, N>;
  };

  // convenience namespace
  namespace Shaders
  {
    using float_ = ShaderExpression<float>;
    using vec2_ = ShaderExpression<vec2>;
    using vec3_ = ShaderExpression<vec3>;
    using vec4_ = ShaderExpression<vec4>;
    using mat3x3_ = ShaderExpression<mat3x3>;
    using mat4x4_ = ShaderExpression<mat4x4>;
    using uint_ = ShaderExpression<uint32_t>;
    using uvec2_ = ShaderExpression<uvec2>;
    using uvec3_ = ShaderExpression<uvec3>;
    using uvec4_ = ShaderExpression<uvec4>;
    using int_ = ShaderExpression<int32_t>;
    using ivec2_ = ShaderExpression<ivec2>;
    using ivec3_ = ShaderExpression<ivec3>;
    using ivec4_ = ShaderExpression<ivec4>;
    using bool_ = ShaderExpression<bool>;
    using bvec2_ = ShaderExpression<bvec2>;
    using bvec3_ = ShaderExpression<bvec3>;
    using bvec4_ = ShaderExpression<bvec4>;

    // construct vector from scalars or vectors
    template <typename... T>
    ShaderExpression<typename ShaderVectorComposeHelper<T...>::Result>
    cvec(ShaderExpression<T> const&... args)
    requires ShaderVectorComposeHelper<T...>::Ok
    {
      return ShaderOperation<ShaderOperationType::Construct, typename ShaderVectorComposeHelper<T...>::Result, T...>(args...);
    }

    // functions

    // partial derivatives
    template <IsFloatScalarOrVector T> ShaderExpression<T> dpdx(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::DPdx, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> dpdy(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::DPdy, T>(a); }

    // arithmetic
    template <IsScalarOrVector T> ShaderExpression<T> abs(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Abs, T>(a); }
    template <IsScalarOrVector T> ShaderExpression<T> sqr(ShaderExpression<T> const& a)
    { return a * a; }
    template <IsFloatScalarOrVector T> ShaderExpression<T> floor(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Floor, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> ceil(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Ceil, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> fract(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Fract, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> sqrt(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Sqrt, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> inverseSqrt(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::InverseSqrt, T>(a); }

    // transcendental
    template <IsFloatScalarOrVector T> ShaderExpression<T> sin(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Sin, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> cos(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Cos, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> tan(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Tan, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> asin(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Asin, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> acos(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Acos, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> atan(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Atan, T>(a); }

    // exponential
    template <IsFloatScalarOrVector T> ShaderExpression<T> pow(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Pow, T>(a, b); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> exp(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Exp, T>(a, b); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> log(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Log, T>(a, b); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> exp2(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Exp2, T>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> log2(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Log2, T>(a); }

    // comparisons
    template <IsScalarOrVector T> ShaderExpression<T> min(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Min, T>(a, b); }
    template <IsScalarOrVector T> ShaderExpression<T> max(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Max, T>(a, b); }
    template <IsScalarOrVector T> ShaderExpression<T> clamp(ShaderExpression<T> const& a, ShaderExpression<T> const& b, ShaderExpression<T> const& c)
    { return ShaderOperation<ShaderOperationType::Clamp, T>(a, b, c); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> mix(ShaderExpression<T> const& a, ShaderExpression<T> const& b, ShaderExpression<T> const& c)
    { return ShaderOperation<ShaderOperationType::Mix, T>(a, b, c); }

    // vectors
    template <size_t n> ShaderExpression<float> dot(ShaderExpression<xvec<float, n>> const& a, ShaderExpression<xvec<float, n>> const& b)
    { return ShaderOperation<ShaderOperationType::Dot, float>(a, b); }
    template <IsFloatScalarOrVector T> ShaderExpression<typename VectorTraits<T>::Scalar> length(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Length, typename VectorTraits<T>::Scalar>(a); }
    template <IsFloatScalarOrVector T> ShaderExpression<typename VectorTraits<T>::Scalar> distance(ShaderExpression<T> const& a, ShaderExpression<T> const& b)
    { return ShaderOperation<ShaderOperationType::Distance, typename VectorTraits<T>::Scalar>(a, b); }
    template <typename T> ShaderExpression<xvec<T, 3>> cross(ShaderExpression<xvec<T, 3>> const& a, ShaderExpression<xvec<T, 3>> const& b)
    { return ShaderOperation<ShaderOperationType::Cross, xvec<T, 3>>(a, b); }
    template <IsFloatScalarOrVector T> ShaderExpression<T> normalize(ShaderExpression<T> const& a)
    { return ShaderOperation<ShaderOperationType::Normalize, T>(a); }
  };
}
