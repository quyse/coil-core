#pragma once

#include "math.hpp"
#include <array>
#include <memory>

namespace Coil
{
  enum class ShaderDataKind : uint8_t
  {
    Scalar,
    Vector,
    Matrix,
    Array,
  };

  struct ShaderDataScalarType;

  struct ShaderDataType
  {
    ShaderDataType(ShaderDataType const&) = delete;
    ShaderDataType(ShaderDataType&&) = delete;

    virtual ShaderDataKind GetKind() const = 0;
    virtual ShaderDataScalarType const& GetBaseScalarType() const = 0;

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

    ShaderDataScalarType(Type t)
    : t(t) {}

    Type t;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Scalar;
    }

    ShaderDataScalarType const& GetBaseScalarType() const override
    {
      return *this;
    }
  };

  struct ShaderDataVectorType : public ShaderDataType
  {
    ShaderDataVectorType(ShaderDataScalarType const& t, size_t n)
    : t(t), n(n) {}

    ShaderDataScalarType const& t;
    size_t n;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Vector;
    }

    ShaderDataScalarType const& GetBaseScalarType() const override
    {
      return t;
    }
  };

  struct ShaderDataMatrixType : public ShaderDataType
  {
    ShaderDataMatrixType(ShaderDataScalarType const& t, size_t n, size_t m)
    : t(t), n(n), m(m) {}

    ShaderDataScalarType const& t;
    size_t n, m;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Matrix;
    }

    ShaderDataScalarType const& GetBaseScalarType() const override
    {
      return t;
    }
  };

  struct ShaderDataArrayType : public ShaderDataType
  {
    ShaderDataArrayType(ShaderDataType const& t, size_t n)
    : t(t), n(n) {}

    ShaderDataType const& t;
    size_t n;

    ShaderDataKind GetKind() const override
    {
      return ShaderDataKind::Array;
    }

    ShaderDataScalarType const& GetBaseScalarType() const override
    {
      return t.GetBaseScalarType();
    }
  };

  // comparison operator supports all kinds of types
  bool operator<(ShaderDataType const& a, ShaderDataType const& b);

  template <typename T>
  struct ShaderDataTypeOfHelper;

  template <>
  struct ShaderDataTypeOfHelper<float>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_float);
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<uint32_t>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_uint);
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<int32_t>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_int);
      return t;
    }
  };
  template <>
  struct ShaderDataTypeOfHelper<bool>
  {
    static ShaderDataScalarType const& GetType()
    {
      static ShaderDataScalarType const t(ShaderDataScalarType::Type::_bool);
      return t;
    }
  };

  template <typename T, size_t n>
  struct ShaderDataTypeOfHelper<xvec<T, n>>
  {
    static ShaderDataVectorType const& GetType()
    {
      static ShaderDataVectorType const t(ShaderDataTypeOfHelper<T>::GetType(), n);
      return t;
    }
  };

  template <typename T, size_t n, size_t m>
  struct ShaderDataTypeOfHelper<xmat<T, n, m>>
  {
    static ShaderDataMatrixType const& GetType()
    {
      static ShaderDataMatrixType const t(ShaderDataTypeOfHelper<T>::GetType(), n, m);
      return t;
    }
  };

  template <typename T, size_t n>
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
  };

  enum class ShaderExpressionType
  {
    // general operation which accepts expressions as arguments
    Operation,
    // reads variable
    Read,
  };

  enum class ShaderOperationType
  {
    Const,
    Cast,
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

  template <typename T>
  struct ShaderExpression
  {
    // init with expression
    template <typename Node>
    ShaderExpression(std::shared_ptr<Node>&& node)
    : node(std::move(node)) {}

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
