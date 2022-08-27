#pragma once

#include "math.hpp"
#include <array>
#include <memory>
#include <type_traits>

namespace Coil
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

    virtual ShaderDataKind GetKind() const = 0;
    virtual uint32_t GetSize() const = 0;

  protected:
    ShaderDataType() = default;
  };

  struct ShaderDataScalarType : public ShaderDataType
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

  struct ShaderDataVectorType : public ShaderDataType
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

  struct ShaderDataMatrixType : public ShaderDataType
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

  struct ShaderDataArrayType : public ShaderDataType
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

  struct ShaderDataStructType : public ShaderDataType
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
  ShaderDataScalarType const& ShaderDataTypeGetScalarType(ShaderDataType const& dataType);

  // comparison operator supports all kinds of types
  bool operator<(ShaderDataType const& a, ShaderDataType const& b);

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
    // uniform buffer
    UniformBuffer,
    // variables have value type and can be written to
    Variable,
  };

  enum class ShaderExpressionType
  {
    // general operation which accepts expressions as arguments
    Operation,
    // reads variable
    Read,
    // reads uniform buffer member
    Uniform,
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
  };

  enum class ShaderStatementType
  {
    Sequence,
    Write,
  };

  enum class ShaderVariableType
  {
    Attribute,
    Interpolant,
    Fragment,
  };

  struct ShaderNode
  {
    virtual ~ShaderNode() {}

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

    // swizzle
    template <size_t n>
    ShaderExpression<typename VectorTraits<xvec<typename VectorTraits<T>::Scalar, n - 1>>::PossiblyScalar> operator[](char const (&swizzle)[n]) const
    {
      return std::make_shared<ShaderOperationSwizzleNode>(ShaderDataTypeOf<typename VectorTraits<xvec<typename VectorTraits<T>::Scalar, n - 1>>::PossiblyScalar>(), node, swizzle);
    }

    // operations
    ShaderExpression operator-() const
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Negate, 1>>(std::array { node });
    }
    friend ShaderExpression operator+(ShaderExpression const& a, ShaderExpression const& b)
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Add, 2>>(std::array { a.node, b.node });
    }
    friend ShaderExpression operator-(ShaderExpression const& a, ShaderExpression const& b)
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Subtract, 2>>(std::array { a.node, b.node });
    }
    friend ShaderExpression operator*(ShaderExpression const& a, ShaderExpression const& b)
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Multiply, 2>>(std::array { a.node, b.node });
    }
    friend ShaderExpression operator/(ShaderExpression const& a, ShaderExpression const& b)
    {
      return std::make_shared<ShaderOperationNodeImpl<T, ShaderOperationType::Divide, 2>>(std::array { a.node, b.node });
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

  struct ShaderStatementNode : public ShaderNode
  {
    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Statement;
    }

    virtual ShaderStatementType GetStatementType() const = 0;
  };

  struct ShaderStatementSequenceNode : public ShaderStatementNode
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

  struct ShaderUniformBufferNode : public ShaderNode
  {
    ShaderUniformBufferNode(ShaderDataStructType const& dataType, uint32_t slotSet, uint32_t slot)
    : dataType(dataType), slotSet(slotSet), slot(slot) {}

    ShaderDataStructType const& dataType;
    uint32_t slotSet;
    uint32_t slot;

    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Variable;
    }
  };

  struct ShaderVariableNode : public ShaderNode
  {
    ShaderNodeType GetNodeType() const
    {
      return ShaderNodeType::Variable;
    }

    virtual ShaderVariableType GetVariableType() const = 0;
    virtual ShaderDataType const& GetDataType() const = 0;
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

  struct ShaderUniformNode : public ShaderExpressionNode
  {
    ShaderUniformNode(std::shared_ptr<ShaderUniformBufferNode> uniformBufferNode, ShaderDataType const& dataType, uint32_t index)
    : uniformBufferNode(std::move(uniformBufferNode)), dataType(dataType), index(index) {}

    std::shared_ptr<ShaderUniformBufferNode> uniformBufferNode;
    ShaderDataType const& dataType;
    uint32_t index;

    ShaderExpressionType GetExpressionType() const override
    {
      return ShaderExpressionType::Uniform;
    }

    ShaderDataType const& GetDataType() const override
    {
      return dataType;
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

  struct ShaderStatementWriteNode : public ShaderStatementNode
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

  struct ShaderVariableAttributeNode : public ShaderVariableNode
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

  struct ShaderVariableInterpolantNode : public ShaderVariableNode
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

  enum class ShaderFragmentBuiltin
  {
    None,
    FragDepth,
  };

  struct ShaderVariableFragmentNode : public ShaderVariableNode
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

  // get type of struct
  template <template <template <typename> typename> typename T>
  ShaderDataStructType const& ShaderDataStructTypeOf()
  {
    // declare mutable members vector
    static std::vector<ShaderDataStructType::Member> members;
    // register members
    static T<ShaderDataStructTypeOfHelper<members>::template Member> const object;
    // finalize type
    static ShaderDataStructType const type = ShaderDataStructTypeOfHelper<members>::Finalize(&object, sizeof(object));

    return type;
  };

  // identity tranform for struct
  template <template <template <typename> typename> typename T>
  using ShaderDataIdentityStruct = T<std::type_identity_t>;

  // struct establishing slot for uniform buffer
  // must be used with static storage duration
  template <template <template <typename> typename> typename T>
  struct ShaderDataUniformStructSlot
  {
    template <template <typename> typename Q>
    using Struct = T<Q>;

    ShaderDataUniformStructSlot(uint32_t slotSet, uint32_t slot)
    : structType(ShaderDataStructTypeOf<T>()),
      node(std::make_shared<ShaderUniformBufferNode>(structType, slotSet, slot))
    {}

    ShaderDataStructType const& structType;
    std::shared_ptr<ShaderUniformBufferNode> node;

    // counter for member initialization
    mutable uint32_t nextMember = 0;
  };

  template <auto& initializer>
  struct ShaderDataUniformStructHelper
  {
    template <typename T>
    struct Member : public ShaderExpression<T>
    {
      Member()
      : ShaderExpression<T>(std::make_shared<ShaderUniformNode>(
          initializer.node,
          ShaderDataTypeOf<T>(),
          initializer.nextMember++
        )) {}
    };
  };

  template <auto& initializer>
  using ShaderDataUniformStruct = typename std::remove_cvref_t<decltype(initializer)>::template Struct<ShaderDataUniformStructHelper<initializer>::template Member>;

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

    ShaderStatement Write(ShaderExpression<T> expression)
    {
      return ShaderStatement(std::make_shared<ShaderStatementWriteNode>(node, expression.node));
    }
  };

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
      (std::is_same_v<Scalar, typename VectorTraits<TT>::Scalar> && ...) &&
      // size requirements
      N >= 2 && N <= 4;
    // result vector type
    using Result = xvec<Scalar, N>;
  };

  // construct vector from scalars or vectors
  template <typename... T>
  ShaderExpression<typename ShaderVectorComposeHelper<T...>::Result>
  cvec(ShaderExpression<T>... args)
  requires ShaderVectorComposeHelper<T...>::Ok
  {
    return std::make_shared<
      ShaderOperationNodeImpl<
        typename ShaderVectorComposeHelper<T...>::Result,
        ShaderOperationType::Construct,
        sizeof...(T)
      >
    >(std::array<std::shared_ptr<ShaderExpressionNode>, sizeof...(T)> { (args.node)... });
  }

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
  };
}
