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

@property (nonatomic, strong) IBOutlet UIButton* addButton;
@property (nonatomic, strong) IBOutlet UIButton* finishButton;
@property (nonatomic, strong) IBOutlet UILabel* infoLabel;

-(IBAction) onAddButtonClicked:(id)sender;
-(IBAction) onFinishButtonClicked:(id)sender;

@property (nonatomic, assign) int stage;

@property (nonatomic, assign) vector_float2 cursor;

@property (nonatomic, assign) NSUInteger maxVerticesCount;
@property (nonatomic, assign) vector_float2* polygonVerticesData;
@property (nonatomic, assign) NSUInteger currentPolygonVerticesCount;
@property (nonatomic, assign) NSUInteger totalVerticesCount;
@property (nonatomic, strong) NSMutableArray<NSNumber* >* polygonSizes;


@end

@implementation ViewController

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"mtkView:drawableSizeWillChange:");
}

- (void)drawInMTKView:(nonnull MTKView *)view {
//    NSLog(@"drawInMTKView:");
    id<MTLCommandBuffer> commandBuffer = [_mtCommandQueue commandBuffer];
    [commandBuffer enqueue];
    
    _mtView.currentRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(1, 1, 1, 1);
    _mtView.clearColor = MTLClearColorMake(0, 0, 0, 1);
    
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:view.currentRenderPassDescriptor];
    [renderEncoder setRenderPipelineState:_primitiveRenderPipeline];
    vector_float2 viewsize = (vector_float2){view.bounds.size.width, view.bounds.size.height};
    float contentScale = view.layer.contentsScale;
    [renderEncoder setViewport:(MTLViewport){0.f, 0.f, viewsize.x * contentScale, viewsize.y * contentScale, 0.f, 1.f}];
    static const vector_float4 greenColor = (vector_float4){0.f, 1.f, 0.f, 1.f};
    static const vector_float4 yellowColor = (vector_float4){1.f, 1.f, 0.f, 1.f};
    static const vector_float4 whiteColor = (vector_float4){1.f, 1.f, 1.f, 1.f};
    [renderEncoder setVertexBytes:&viewsize length:sizeof(viewsize) atIndex:ViewportSlot];
    
    vector_float2 crossLines[] = {
        //*
        (vector_float2){-1.f, _cursor.y},
        (vector_float2){1.f, _cursor.y},
        
        (vector_float2){_cursor.x, -1.f},
        (vector_float2){_cursor.x, 1.f},
        /*/
        (vector_float2){-1.f, 0.f},
        (vector_float2){1.f, 0.f},
        
        //(vector_float4){0.f, -1.f, 0.f, 1.f},
        //(vector_float4){0.f, 1.f, 0.f, 1.f},
        //*/
    };
    [renderEncoder setVertexBytes:crossLines length:sizeof(crossLines) atIndex:VertexSlot];
    [renderEncoder setFragmentBytes:&yellowColor length:sizeof(vector_float4) atIndex:ColorSlot];
    [renderEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:sizeof(crossLines)/sizeof(crossLines[0]) instanceCount:1];

    [renderEncoder setVertexBytes:_polygonVerticesData length:sizeof(vector_float2) * (_totalVerticesCount + 1) atIndex:VertexSlot];
    [renderEncoder setFragmentBytes:&greenColor length:sizeof(vector_float4) atIndex:ColorSlot];
    int vertexIndex = 0;
    if (_polygonSizes.count > 0)
    {
        uint32_t* endLineIndices = (uint32_t*) malloc(sizeof(uint32_t) * 2 * _polygonSizes.count);
        uint32_t* pEndLineIndices = endLineIndices;
        for (NSNumber* polygonSize in _polygonSizes)
        {
            uint32_t verticesCount = (uint32_t)[polygonSize integerValue];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeLineStrip vertexStart:vertexIndex vertexCount:verticesCount instanceCount:1];
            *pEndLineIndices++ = vertexIndex;
            *pEndLineIndices++ = vertexIndex + verticesCount - 1;
            vertexIndex += verticesCount;
        }
        id<MTLBuffer> indices = [view.device newBufferWithBytes:endLineIndices length:(sizeof(uint32_t) * 2 * _polygonSizes.count) options:MTLResourceOptionCPUCacheModeDefault];
        free(endLineIndices);
        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:(2 * _polygonSizes.count) indexType:MTLIndexTypeUInt32 indexBuffer:indices indexBufferOffset:0];
    }
    if (_currentPolygonVerticesCount > 0)
    {
        [renderEncoder drawPrimitives:MTLPrimitiveTypeLineStrip vertexStart:vertexIndex vertexCount:_currentPolygonVerticesCount + 1 instanceCount:1];
        if (_currentPolygonVerticesCount > 2)
        {
            [renderEncoder setFragmentBytes:&whiteColor length:sizeof(vector_float4) atIndex:ColorSlot];
            vector_float2 line[] = {_polygonVerticesData[vertexIndex], _polygonVerticesData[vertexIndex + _currentPolygonVerticesCount]};
            [renderEncoder setVertexBytes:line length:sizeof(line) atIndex:VertexSlot];
            [renderEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:2];
        }
    }
    
    [renderEncoder endEncoding];

    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
    
}

-(void) onPanGestureRecognized:(UIPanGestureRecognizer*)recognizer {
    CGPoint touchPoint = [recognizer locationInView:recognizer.view];
    _cursor = (vector_float2){(touchPoint.x / recognizer.view.bounds.size.width - 0.5f) * 2.f, (0.5f - touchPoint.y / recognizer.view.bounds.size.height) * 2.f};
    _polygonVerticesData[_totalVerticesCount] = _cursor;
}

-(IBAction) onAddButtonClicked:(id)sender {
    switch (_stage)
    {
        case 0:
        {
            _polygonVerticesData[_totalVerticesCount++] = _cursor;
            _polygonVerticesData[_totalVerticesCount] = _cursor;
            _currentPolygonVerticesCount++;
            _finishButton.enabled = (_currentPolygonVerticesCount > 2);
            _finishButton.hidden = !(_currentPolygonVerticesCount > 2);
            if (_totalVerticesCount == _maxVerticesCount)
            {
                _addButton.enabled = NO;
            }
        }
            break;
            
        default:
            break;
    }
}

-(IBAction) onFinishButtonClicked:(id)sender {
    switch (_stage)
    {
        case 0:
        {
            [_polygonSizes addObject:@(_currentPolygonVerticesCount)];
            _currentPolygonVerticesCount = 0;
            _finishButton.enabled = NO;
        }
            break;
            
        default:
            break;
    }
}

-(void) dealloc {
    free(_polygonVerticesData);
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
    
    _stage = 0;
    //_cursor = (vector_float2){self.view.bounds.size.width / 2, self.view.bounds.size.height / 2};
    _cursor = (vector_float2){0.f, 0.f};
    _maxVerticesCount = 1024;
    _polygonSizes = [[NSMutableArray alloc] init];
    _currentPolygonVerticesCount = 0;
    _totalVerticesCount = 0;
    _polygonVerticesData = (vector_float2*) malloc(sizeof(vector_float2) * _maxVerticesCount);
    
    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanGestureRecognized:)];
    [_mtView addGestureRecognizer:panRecognizer];
}


@end
