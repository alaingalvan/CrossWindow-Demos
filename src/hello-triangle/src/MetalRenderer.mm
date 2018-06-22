#include "Renderer.h"

Renderer::Renderer(xwin::Window& window)
{
  initializeAPI(window);
  initializeResources();
  setupCommands();
  tStart = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer()
{
  xgfx::unsetContext(mState);
  xgfx::destroyContext(mState);
}

void Renderer::initializeAPI(xwin::Window& window)
{
  mLayer = xgfx::createMetalLayer(window);
  xwin::WindowDelegate del = window.getDelagate();

  mDevice = del.view.device;

  // Load all the shader files with a .metal file extension in the project
  defaultLibrary = [mDevice newDefaultLibrary];

  // Load the vertex function from the library
  vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];

  // Load the fragment function from the library
  fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

  // Configure a pipeline descriptor that is used to create a pipeline state
  MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
  pipelineStateDescriptor.label = @"Simple Pipeline";
  pipelineStateDescriptor.vertexFunction = vertexFunction;
  pipelineStateDescriptor.fragmentFunction = fragmentFunction;
  pipelineStateDescriptor.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat;

  mPipelineState = [mDevice newRenderPipelineStateWithDescriptor:pipelineStateDescriptor
                                                            error:&error];
  if (!mPipelineState)
  {
      // Pipeline State creation could fail if we haven't properly set up our pipeline descriptor.
      //  If the Metal API validation is enabled, we can find out more information about what
      //  went wrong.  (Metal API validation is enabled by default when a debug build is run
      //  from Xcode)
      NSLog(@"Failed to created pipeline state, error %@", error);
      return nil;
  }

  // Create the command queue
  mCommandQueue = [mDevice newCommandQueue];
}

void Renderer::initializeResources()
{
     static const AAPLVertex triangleVertices[] =
    {
        // 2D positions,    RGBA colors
        { {  250,  -250 }, { 1, 0, 0, 1 } },
        { { -250,  -250 }, { 0, 1, 0, 1 } },
        { {    0,   250 }, { 0, 0, 1, 1 } },
    };

    // Create a new command buffer for each render pass to the current drawable
    mCommandBuffer = [mCommandQueue mCommandBuffer];
    mCommandBuffer.label = @"MyCommand";

    // Obtain a renderPassDescriptor generated from the view's drawable textures
    xwin::WindowDelegate del = window.getDelagate();

    NSView* view = del.view;
    
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil)
    {
        // Create a render command encoder so we can render into something
        id<MTLRenderCommandEncoder> renderEncoder =
        [mCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

        // Set the region of the drawable to which we'll draw.
        [renderEncoder setViewport:(MTLViewport){0.0, 0.0, _viewportSize.x, _viewportSize.y, -1.0, 1.0 }];

        [renderEncoder setRenderPipelineState:_pipelineState];

        [renderEncoder setVertexBytes:triangleVertices
                               length:sizeof(triangleVertices)
                              atIndex:AAPLVertexInputIndexVertices];

        [renderEncoder setVertexBytes:&_viewportSize
                               length:sizeof(_viewportSize)
                              atIndex:AAPLVertexInputIndexViewportSize];

        // Draw the 3 vertices of our triangle
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                          vertexStart:0
                          vertexCount:3];

        [renderEncoder endEncoding];

        [mCommandBuffer presentDrawable:view.currentDrawable];
    }

}

Renderer::render()
{
    // Framelimit set to 60 fps
    tEnd = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
    if (time < (1000.0f / 60.0f))
    { return; }
    tStart = std::chrono::high_resolution_clock::now();

    mElapsedTime += 0.001f * time;
    mElapsedTime = fmodf(mElapsedTime, 6.283185307179586f);
    
    // Finalize rendering here & push the command buffer to the GPU
    [mCommandBuffer commit];
}