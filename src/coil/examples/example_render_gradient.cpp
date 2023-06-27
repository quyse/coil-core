#include <coil/appidentity.hpp>
#include <coil/sdl.hpp>
#include <coil/sdl_vulkan.hpp>
#include <coil/vulkan.hpp>
#include <coil/render.hpp>
#include <coil/render_canvas.hpp>
#include <coil/entrypoint.hpp>
#include <type_traits>

using namespace Coil;

int COIL_ENTRY_POINT(std::vector<std::string>&& args)
{
  AppIdentity::GetInstance().Name() = "coil_core_example_render_gradient";

  Book book;

  SdlVulkanSystem::Init();
  auto& windowSystem = book.Allocate<SdlWindowSystem>();

  Window& window = windowSystem.CreateWindow(book, "coil_core_example_render_gradient", { 1024, 768 });
  window.SetLoopOnlyVisible(true);

  GraphicsSystem& graphicsSystem = VulkanSystem::Create(book, window,
  {
    .render = true,
  });
  GraphicsDevice& graphicsDevice = graphicsSystem.CreateDefaultDevice(book);

  Book& graphicsBook = graphicsDevice.GetBook();
  GraphicsPool& pool = graphicsDevice.CreatePool(graphicsBook, 16 * 1024 * 1024);

  Canvas canvas(graphicsDevice);
  canvas.Init(graphicsBook, pool);

  GraphicsShader& shader = graphicsDevice.CreateShader(book, [&]() -> GraphicsShaderRoots
  {
    using namespace Shaders;

    auto const& [vertex] = Canvas::GetScreenQuadVertexLayout().slots;

    ShaderSampledImage<float, 2> image(0, 1);

    auto vInPosition = *vertex.position;

    auto iOutPosition = ShaderInterpolantBuiltinPosition();
    auto iPosition = ShaderInterpolant<vec2>(0);

    auto vertexProgram = (
      iOutPosition.Write(cvec(vInPosition * float_(0.5f), float_(0), float_(1))),
      iPosition.Write(vInPosition * float_(0.5f) + vec2_(0.5f, 0.5f))
    );

    auto fOutColor = ShaderFragment<vec4>(0);

    auto fragmentProgram = (
      fOutColor.Write(cvec(swizzle(*iPosition, "xy"), float_(0), float_(1)))
    );

    return
    {
      .vertex = vertexProgram,
      .fragment = fragmentProgram,
    };
  }());

  GraphicsPipelineLayout& pipelineLayout = [&]() -> auto&
  {
    GraphicsShader* shaders[] = { &shader };
    return graphicsDevice.CreatePipelineLayout(book, shaders);
  }();

  GraphicsPass* pPass = nullptr;

  GraphicsPipeline* pPipeline = nullptr;
  std::vector<GraphicsFramebuffer*> pFramebuffers;

  GraphicsPresenter& graphicsPresenter = graphicsDevice.CreateWindowPresenter(book, pool, window,
    [&](GraphicsPresentConfig const& presentConfig, uint32_t imagesCount)
    {
      canvas.SetSize(presentConfig.size);

      {
        GraphicsPassConfig passConfig;
        auto colorRef = passConfig.AddAttachment(GraphicsPassConfig::ColorAttachmentConfig
        {
          .format = presentConfig.pixelFormat,
          .clearColor = { 0.5f, 0.5f, 0.5f, 0 },
        });
        colorRef->keepAfter = true;
        auto subPass = passConfig.AddSubPass();
        subPass->UseColorAttachment(colorRef, 0);
        pPass = &graphicsDevice.CreatePass(presentConfig.book, passConfig);
      }

      pPipeline = &canvas.CreateQuadPipeline(presentConfig.book, pipelineLayout, *pPass, 0, shader);

      pFramebuffers.assign(imagesCount, nullptr);
    },
    [&](GraphicsPresentConfig const& presentConfig, uint32_t imageIndex, GraphicsImage& image)
    {
      GraphicsImage* images[] = { &image };
      pFramebuffers[imageIndex] = &graphicsDevice.CreateFramebuffer(presentConfig.book, *pPass, images, presentConfig.size);
    });

  InputManager& inputManager = window.GetInputManager();

  RenderContext renderContext;

  window.Run([&]()
  {
    auto& inputFrame = inputManager.GetCurrentFrame();
    while(auto const* event = inputFrame.NextEvent())
    {
      std::visit([&]<typename E1>(E1 const& event)
      {
        if constexpr(std::same_as<E1, InputKeyboardEvent>)
        {
          std::visit([&]<typename E2>(E2 const& event)
          {
            if constexpr(std::same_as<E2, InputKeyboardKeyEvent>)
            {
              if(event.isPressed)
              {
                switch(event.key)
                {
                case InputKey::Escape:
                  window.Stop();
                  break;
                default:
                  break;
                }
              }
            }
          }, event);
        }
      }, *event);
    }

    GraphicsFrame& frame = graphicsPresenter.StartFrame();
    frame.Pass(*pPass, *pFramebuffers[frame.GetImageIndex()], [&](GraphicsSubPassId subPassId, GraphicsContext& context)
    {
      renderContext.Begin(context);
      RenderPipelineKnob(*pPipeline).Apply(renderContext);
      RenderMeshKnob(canvas.GetQuadMesh()).Apply(renderContext);
      renderContext.EndInstance();
      renderContext.Flush();
    });
    frame.EndFrame();
  });

  return 0;
}
