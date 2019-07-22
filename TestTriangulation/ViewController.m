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
#import <simd/simd.h>

bool areLinesIntersect(vector_float2 line0s, vector_float2 line0e, vector_float2 line1s, vector_float2 line1e) {
    const float Epsilon = 0.0000000000001;
    const float A0 = line0e.y - line0s.y;
    const float B0 = line0s.x - line0e.x;
    const float C0 = line0e.x * line0s.y - line0s.x * line0e.y;
    
    const float A1 = line1e.y - line1s.y;
    const float B1 = line1s.x - line1e.x;
    const float C1 = line1e.x * line1s.y - line1s.x * line1e.y;
    
    const float D = A0 * B1 - A1 * B0;
    const float E = B1 * C0 - B0 * C1;
    const float F = A0 * C1 - A1 * C0;
    if (fabsf(D) <= Epsilon)
    {// Parallel
        if (fabsf(E) <= Epsilon && fabsf(F) <= Epsilon)
        {// On the same line
            float maxX0, minX0, maxX1, minX1;
            if (line0s.x > line0e.x)
            {
                maxX0 = line0s.x;
                minX0 = line0e.x;
            }
            else
            {
                maxX0 = line0e.x;
                minX0 = line0s.x;
            }
            if (line1s.x > line1e.x)
            {
                maxX1 = line1s.x;
                minX1 = line1e.x;
            }
            else
            {
                maxX1 = line1e.x;
                minX1 = line1s.x;
            }
            if (maxX0 < minX1 || maxX1 < minX0)
                return false;
            
            float maxY0, minY0, maxY1, minY1;
            if (line0s.y > line0e.y)
            {
                maxY0 = line0s.y;
                minY0 = line0e.y;
            }
            else
            {
                maxY0 = line0e.y;
                minY0 = line0s.y;
            }
            if (line1s.y > line1e.y)
            {
                maxY1 = line1s.y;
                minY1 = line1e.y;
            }
            else
            {
                maxY1 = line1e.y;
                minY1 = line1s.y;
            }
            if (maxY0 < minY1 || maxY1 < minY0)
                return false;
            
            return true;
        }
        else
        {// Parallel and Apart
            return false;
        }
    }
    else
    {
        vector_float2 intersection = (vector_float2){E/D, F/D};
        float signX0 = (intersection.x - line0s.x) * (intersection.x - line0e.x);
        float signX1 = (intersection.x - line1s.x) * (intersection.x - line1e.x);
        if (signX0 <= 0 && signX1 <= 0)
            return true;
        
        float signY0 = (intersection.y - line0s.y) * (intersection.y - line0e.y);
        float signY1 = (intersection.y - line1s.y) * (intersection.y - line1e.y);
        if (signY0 <= 0 && signY1 <= 0)
            return true;
        
        return false;
    }
}

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
