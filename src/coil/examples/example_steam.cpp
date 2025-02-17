#include <coil/base_meta.hpp>
#include <coil/entrypoint.hpp>
#include <coroutine>
#include <iostream>

import coil.core.appidentity;
import coil.core.assets.structs;
import coil.core.assets;
import coil.core.base.generator;
import coil.core.base.util;
import coil.core.base;
import coil.core.fs;
import coil.core.graphics.shaders;
import coil.core.graphics;
import coil.core.image.compress;
import coil.core.image.png;
import coil.core.json;
import coil.core.math;
import coil.core.platform;
import coil.core.player_input.combined;
import coil.core.player_input.native;
import coil.core.player_input;
import coil.core.render.canvas;
import coil.core.render;
import coil.core.sdl.vulkan;
import coil.core.sdl;
import coil.core.steam;
import coil.core.tasks;
import coil.core.vulkan;

using namespace Coil;

COIL_META_STRUCT(InGameActionSet)
{
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Button, fire);
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Button, exit);
  COIL_META_STRUCT_FIELD(PlayerInputActionType::Analog, move);
};

COIL_META_STRUCT(Assets)
{
  COIL_META_STRUCT_FIELD(Coil::GraphicsImage*, texture);
};

int COIL_ENTRY_POINT(std::vector<std::string> args)
{
  AppIdentity::GetInstance().Name() = "coil_core_example_steam";

  TaskEngine::GetInstance().AddThread();

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
  GraphicsPool& graphicsPool = graphicsDevice.CreatePool(graphicsBook, 16 * 1024 * 1024);

  auto& graphicsAssetManager = book.Allocate<GraphicsAssetManager>(graphicsDevice, graphicsPool);

  AssetManager assetManager =
  {
    FileAssetLoader(),
    PngAssetLoader(),
    ImageCompressAssetLoader(),
    TextureAssetLoader(graphicsAssetManager),
    SamplerAssetLoader(graphicsAssetManager),
  };
  assetManager.SetJsonContext(JsonFromBuffer(File::MapRead(book, "example_assets.json")));

  Assets<AssetStructAdapter> assets;
  assets.SelfLoad(book, assetManager).Get();

  Canvas canvas(graphicsDevice);
  canvas.Init(graphicsBook, graphicsPool);

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

  GraphicsPresenter& graphicsPresenter = graphicsDevice.CreateWindowPresenter(book, graphicsPool, window,
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
  actionSetRegistration.Register(playerInputManager, "InGame");

  auto pControllers = playerInputManager.GetControllers();

  RenderContext renderContext{RenderContext::RenderType::Instances};

  window.RunGenerator([&]() -> Generator<std::tuple<>>
  {
    for(;;)
    {
      bool firing = false;

      steam.Update();
      playerInputManager.Update();

      for(auto [controllerId, _] : *pControllers)
      {
        InGameActionSet<PlayerInputActionSetAdapter> actionSet;
        actionSet.Register(playerInputManager, actionSetRegistration, controllerId);
        actionSet.Activate(playerInputManager);
        firing |= actionSet.fire.sigIsPressed.Get();
        if(actionSet.exit.sigIsPressed.Get())
        {
          co_return;
        }
      }

      GraphicsFrame& frame = graphicsPresenter.StartFrame();
      graphicsAssetManager.RunContextTasks(frame.GetContext());
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
