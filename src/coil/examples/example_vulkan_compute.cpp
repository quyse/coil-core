#include <coil/entrypoint.hpp>
#include <random>
#include <iostream>

import coil.core.base;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.math.debug;
import coil.core.math;
import coil.core.vulkan;

using namespace Coil;

template <template <typename> typename T>
struct InputStruct
{
  T<mat<4, 4>> w;
  T<vec<4>> x;
};

template <template <typename> typename T>
struct OutputStruct
{
  T<vec<4>> r;
};

static std::mt19937_64 rnd;

float Random()
{
  return float(rnd() - rnd.min()) / float(rnd.max() - rnd.min());
}

template <size_t n>
vec<n> Random()
{
  vec<n> r;
  for(size_t i = 0; i < n; ++i)
    r(i) = Random();
  return r;
}

template <size_t n, size_t m>
mat<n, m> Random()
{
  mat<n, m> r;
  for(size_t i = 0; i < n; ++i)
    for(size_t j = 0; j < m; ++j)
      r(i, j) = Random();
  return r;
}

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  Book book;
  auto& graphicsSystem = VulkanSystem::Create(book,
  {
    .compute = true,
  });
  auto& graphicsDevice = graphicsSystem.CreateDefaultDevice(book);
  auto& graphicsPool = graphicsDevice.CreatePool(book, 0x10000);
  auto& graphicsComputer = graphicsDevice.CreateComputer(book, graphicsPool);

  auto& shader = graphicsDevice.CreateShader(book, [&]() -> GraphicsShaderRoots
  {
    using namespace Shaders;

    auto iOutPosition = ShaderInterpolantBuiltinPosition();

    auto const& input = GetShaderDataStructBuffer<InputStruct, 0, 0, ShaderBufferType::Storage>();
    auto const& output = GetShaderDataStructBuffer<OutputStruct, 0, 1, ShaderBufferType::Storage>();
    auto computeProgram = (
      output.r.Write(*input.w * *input.x)
    );

    return
    {
      .compute = computeProgram,
    };
  }());
  GraphicsShader* shaders[] = { &shader };
  auto& pipelineLayout = graphicsDevice.CreatePipelineLayout(book, shaders);
  auto& pipeline = graphicsDevice.CreatePipeline(book, pipelineLayout, shader);

  ShaderDataIdentityStruct<InputStruct> input =
  {
    .w = Random<4, 4>(),
    .x = Random<4>(),
  };
  GraphicsStorageBuffer& inputBuffer = graphicsDevice.CreateStorageBuffer(book, graphicsPool, Buffer(&input, sizeof(input)));

  ShaderDataIdentityStruct<OutputStruct> output;
  GraphicsStorageBuffer& outputBuffer = graphicsDevice.CreateStorageBuffer(book, graphicsPool, Buffer(sizeof(output)));

  graphicsComputer.Compute([&](GraphicsContext& context)
  {
    context.BindPipeline(pipeline);
    context.BindStorageBuffer(0, 0, inputBuffer);
    context.BindStorageBuffer(0, 1, outputBuffer);
    context.Dispatch({ 1, 1, 1});
  });

  graphicsDevice.GetStorageBufferData(outputBuffer, Buffer(&output, sizeof(output)));

  std::cout << "should be:\n" << (input.w * input.x) << "\n";
  std::cout << "got:\n" << output.r << "\n";

  return 0;
}
