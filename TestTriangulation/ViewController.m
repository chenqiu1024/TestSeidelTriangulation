//
//  ViewController.m
//  TestTriangulation
//
//  Created by Dom Chiu on 2019/7/21.
//  Copyright © 2019 Dom Chiu. All rights reserved.
//

#import "ViewController.h"
#import "ShaderDefines.h"
#import <MetalKit/MetalKit.h>

@interface ViewController () <MTKViewDelegate>

@property (nonatomic, strong) MTKView* mtView;
@property (nonatomic, strong) id<MTLCommandQueue> mtCommandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> primitiveRenderPipeline;

@end

@implementation ViewController

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"mtkView:drawableSizeWillChange:");
}

- (void)drawInMTKView:(nonnull MTKView *)view {
    NSLog(@"drawInMTKView:");
    id<MTLCommandBuffer> commandBuffer = [_mtCommandQueue commandBuffer];
    [commandBuffer enqueue];
    
    _mtView.currentRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 1, 1, 1);
    _mtView.clearColor = MTLClearColorMake(0, 1, 0, 1);
    
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:view.currentRenderPassDescriptor];
    [renderEncoder setViewport:(MTLViewport){0.f, 0.f, view.bounds.size.width, view.bounds.size.height, 0.f, 1.f}];
    [renderEncoder setRenderPipelineState:_primitiveRenderPipeline];
    vector_float2 viewsize = (vector_float2){view.bounds.size.width, view.bounds.size.height};
    vector_float2 vertices[] = {(vector_float2){0.f, viewsize.y},
        (vector_float2){0.f, 0.f},
        (vector_float2){viewsize.x, viewsize.y},
        (vector_float2){viewsize.x, 0.f},
    };
    vector_float4 color = (vector_float4){1.f, 0.f, 0.f, 1.f};
    [renderEncoder setVertexBytes:&viewsize length:sizeof(viewsize) atIndex:ViewportSlot];
    [renderEncoder setVertexBytes:vertices length:sizeof(vertices) atIndex:VertexSlot];
    [renderEncoder setFragmentBytes:&color length:sizeof(color) atIndex:ColorSlot];
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4 instanceCount:1];
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
    
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    _mtView = (MTKView*) self.view;
    _mtView.delegate = self;
    _mtView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    
    _mtView.device = MTLCreateSystemDefaultDevice();
    _mtCommandQueue = [_mtView.device newCommandQueue];
    id<MTLLibrary> mtLibrary = [_mtView.device newDefaultLibrary];
    id<MTLFunction> vertexFunction = [mtLibrary newFunctionWithName:@"TrivialVertex2DFunction"];
    id<MTLFunction> fragmentFunction = [mtLibrary newFunctionWithName:@"TrivialFragmentFunction"];
    MTLRenderPipelineDescriptor* renderPipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    renderPipelineDesc.colorAttachments[0].pixelFormat = _mtView.colorPixelFormat;
    renderPipelineDesc.colorAttachments[0].blendingEnabled = NO;
    renderPipelineDesc.vertexFunction = vertexFunction;
    renderPipelineDesc.fragmentFunction = fragmentFunction;
    
    _primitiveRenderPipeline = [_mtView.device newRenderPipelineStateWithDescriptor:renderPipelineDesc error:nil];
    
}


@end
