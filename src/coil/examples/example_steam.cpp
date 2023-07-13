#include <coil/appidentity.hpp>
#include <coil/entrypoint.hpp>
#include <coil/fs.hpp>
#include <coil/player_input_combined.hpp>
#include <coil/player_input_native.hpp>
#include <coil/render.hpp>
#include <coil/render_canvas.hpp>
#include <coil/sdl.hpp>
#include <coil/sdl_vulkan.hpp>
#include <coil/steam.hpp>
#include <coil/util.hpp>
#include <coil/util_generator.hpp>
#include <coil/vulkan.hpp>
#include <iostream>

using namespace Coil;

COIL_META_STRUCT(InGameActionSet)
{
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Button, fire);
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Button, exit);
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Analog, move);
};

int COIL_ENTRY_POINT(std::vector<std::string>&& args)
{
  AppIdentity::GetInstance().Name() = "coil_core_example_steam";

  Book book;

  Steam steam;

  SdlVulkanSystem::Init();
  auto& windowSystem = book.Allocate<SdlWindowSystem>();

  Window& window = windowSystem.CreateWindow(book, "coil_core_example_steam", { 1024, 768 });
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

  auto& inputManager = window.GetInputManager();
  auto& nativePlayerInputManager = book.Allocate<NativePlayerInputManager>(inputManager);
  nativePlayerInputManager.SetMapping(JsonDecode<NativePlayerInputMapping>(JsonFromBuffer(File::MapRead(book, "example_native_player_input_mapping.json"))));

  PlayerInputManager& playerInputManager = steam
    ? static_cast<PlayerInputManager&>(book.Allocate<CombinedPlayerInputManager<NativePlayerInputManager, SteamPlayerInputManager>>(
        nativePlayerInputManager,
        book.Allocate<SteamPlayerInputManager>()
      ))
    : static_cast<PlayerInputManager&>(nativePlayerInputManager)
  ;
  InGameActionSet<PlayerInputActionSetRegistrationAdapter> actionSetRegistration;
  actionSetRegistration.Register(playerInputManager);
  auto actionSetId = playerInputManager.GetActionSetId("InGame");

  RenderContext renderContext;

  window.RunGenerator([&]() -> Generator<std::tuple<>>
  {
    for(;;)
    {
      bool firing = false;

      steam.Update();
      playerInputManager.Update();

      {
        auto const& controllersIds = playerInputManager.GetControllersIds();
        for(auto controllerId : controllersIds)
        {
          playerInputManager.ActivateActionSet(controllerId, actionSetId);
          InGameActionSet<PlayerInputActionSetStateAdapter> actionSetState;
          actionSetState.Update(playerInputManager, actionSetRegistration, controllerId);
          firing |= actionSetState.fire.isPressed;
          if(actionSetState.exit.isPressed)
          {
            co_return;
          }
        }
      }

      GraphicsFrame& frame = graphicsPresenter.StartFrame();
      frame.Pass(*pPass, *pFramebuffers[frame.GetImageIndex()], [&](GraphicsSubPassId subPassId, GraphicsContext& context)
      {
        renderContext.Begin(context);
        if(firing)
        {
          RenderPipelineKnob(*pPipeline).Apply(renderContext);
          RenderMeshKnob(canvas.GetQuadMesh()).Apply(renderContext);
          renderContext.EndInstance();
        }
        renderContext.Flush();
      });
      frame.EndFrame();

      co_yield {};
    }
  });

  return 0;
}
