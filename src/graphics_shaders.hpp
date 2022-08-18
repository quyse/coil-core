#pragma once

#include "math.hpp"
#include <array>
#include <memory>

namespace Coil
{
  // data type used in shaders
  enum class ShaderDataType
  {
    _float,
    _vec2,
    _vec3,
    _vec4,
    _mat3x3,
    _mat4x4,
    _uint,
    _uvec2,
    _uvec3,
    _uvec4,
    _int,
    _ivec2,
    _ivec3,
    _ivec4,
    _bool,
    _bvec2,
    _bvec3,
    _bvec4
  };

  // data type info
  struct ShaderDataTypeInfo
  {
    ShaderDataType baseDataType;
  };

  ShaderDataTypeInfo const& ShaderDataTypeInfoOf(ShaderDataType dataType);

  template <typename T>
  ShaderDataType ShaderDataTypeOf();

#define S(T, t) \
  template <> \
  inline ShaderDataType ShaderDataTypeOf<T>() \
  { \
    return ShaderDataType::_##t; \
  }

  S(float, float)
  S(vec2, vec2)
  S(vec3, vec3)
  S(vec4, vec4)
  S(mat3x3, mat3x3)
  S(mat4x4, mat4x4)
  S(uint32_t, uint)
  S(uvec2, uvec2)
  S(uvec3, uvec3)
  S(uvec4, uvec4)
  S(int32_t, int)
  S(ivec2, ivec2)
  S(ivec3, ivec3)
  S(ivec4, ivec4)
  S(bool, bool)
  S(bvec2, bvec2)
  S(bvec3, bvec3)
  S(bvec4, bvec4)

#undef S

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
    virtual ShaderDataType GetDataType() const = 0;
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

    ShaderDataType GetDataType() const override
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
    virtual ShaderDataType GetDataType() const = 0;
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

    ShaderDataType GetDataType() const override
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
    ShaderVariableAttributeNode(ShaderDataType dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableAttributeNode(ShaderDataType dataType, ShaderAttributeBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType dataType;
    uint32_t location = -1;
    ShaderAttributeBuiltin builtin = ShaderAttributeBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Attribute;
    }

    ShaderDataType GetDataType() const override
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
    ShaderVariableInterpolantNode(ShaderDataType dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableInterpolantNode(ShaderDataType dataType, ShaderInterpolantBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType dataType;
    uint32_t location = -1;
    ShaderInterpolantBuiltin builtin = ShaderInterpolantBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Interpolant;
    }

    ShaderDataType GetDataType() const override
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
    ShaderVariableFragmentNode(ShaderDataType dataType, uint32_t location)
    : dataType(dataType), location(location) {}
    ShaderVariableFragmentNode(ShaderDataType dataType, ShaderFragmentBuiltin builtin)
    : dataType(dataType), builtin(builtin) {}

    ShaderDataType dataType;
    uint32_t location = -1;
    ShaderFragmentBuiltin builtin = ShaderFragmentBuiltin::None;

    ShaderVariableType GetVariableType() const override
    {
      return ShaderVariableType::Fragment;
    }

    ShaderDataType GetDataType() const override
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
