//
//  ViewController.m
//  TestTriangulation
//
//  Created by Dom Chiu on 2019/7/21.
//  Copyright © 2019 Dom Chiu. All rights reserved.
//

#import "ViewController.h"
#import "ShaderDefines.h"
#import "interface.h"
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
    const float E = B0 * C1 - B1 * C0;
    const float F = A1 * C0 - A0 * C1;
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
            {// On the same line but apart
                return false;
            }
            
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
            {// On the same line but apart}
                return false;
            }
            
            return true;
        }
        else
        {// Parallel and Not on the same line
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

bool isLinesIntersectWithLineStrip(vector_float2 line0s, vector_float2 line0e, const vector_float2* lines, size_t linesCount) {
    const vector_float2* pPoint0 = lines;
    for (size_t i=linesCount; i>0; --i)
    {
        if (areLinesIntersect(line0s, line0e, pPoint0[0], pPoint0[1])) return true;
        pPoint0++;
    }
    return false;
}

bool isLinesIntersectWithLines(vector_float2 line0s, vector_float2 line0e, const vector_float2* points, const uint32_t* indices, size_t linesCount) {
    const uint32_t* pIndex = indices;
    for (size_t i=linesCount; i>0; --i)
    {
        if (areLinesIntersect(line0s, line0e, points[pIndex[0]], points[pIndex[1]])) return true;
        pIndex += 2;
    }
    return false;
}

bool isPointInPolygon(vector_float2 point, const vector_float2* polygonVertices, size_t polygonSize) {
    int signSum = 0;
    const vector_float2* pVertex = polygonVertices;
    vector_float2 p0 = pVertex[polygonSize - 1], p1 = pVertex[0];
    for (size_t i = polygonSize; i > 0; --i, p0 = *pVertex++, p1 = *pVertex)
    {
        if (p0.y == p1.y) continue;
        
        float dY0 = point.y - p0.y;
        float dY1 = p1.y - point.y;
        if (dY0 * dY1 < 0) continue;
        
        // (x - x0) / (x1 - x0) = (y - y0) / (y1 - y0)
        float intersectX = p0.x + (point.y - p0.y) * (p1.x - p0.x) / (p1.y - p0.y);
        if (intersectX > point.x) continue;
        
        if (dY0 > 0)
            signSum++;
        else if (dY0 < 0)
            signSum--;
        
        if (dY1 > 0)
            signSum++;
        else if (dY1 < 0)
            signSum--;
    }
    return (signSum != 0);
}

bool isPolygonClockwise(const vector_float2* vertices, size_t verticesCount) {
    float leftMostX = vertices[0].x;
    size_t leftMostIndex = 0;
    for (size_t i=1; i<verticesCount; ++i)
    {
        if (vertices[i].x < leftMostX)
        {
            leftMostX = vertices[i].x;
            leftMostIndex = i;
        }
    }
    vector_float2 vAnchor = vertices[leftMostIndex];
    vector_float2 v0 = leftMostIndex > 0 ? vertices[leftMostIndex - 1] : vertices[verticesCount - 1];
    vector_float2 v1 = leftMostIndex < verticesCount - 1 ? vertices[leftMostIndex + 1] : vertices[0];
    // Cross Product : Z = X1 * Y2 - X2 * Y1
    float x0 = vAnchor.x - v0.x, x1 = v1.x - vAnchor.x;
    float y0 = vAnchor.y - v0.y, y1 = v1.y - vAnchor.y;
    float crossProductZ = x0 * y1 - x1 * y0;
    return (crossProductZ <= 0);
}

@interface ViewController () <MTKViewDelegate>

@property (nonatomic, strong) MTKView* mtView;
@property (nonatomic, strong) id<MTLCommandQueue> mtCommandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> primitiveRenderPipeline;

@property (nonatomic, strong) IBOutlet UIButton* addButton;
@property (nonatomic, strong) IBOutlet UIButton* finishButton;
@property (nonatomic, strong) IBOutlet UIButton* triangulateButton;
@property (nonatomic, strong) IBOutlet UILabel* infoLabel;
@property (nonatomic, strong) IBOutlet UISwitch* fillSwitch;
@property (nonatomic, strong) IBOutlet UIButton* profileButton;
@property (nonatomic, strong) IBOutlet UILabel* profileLabel;

-(IBAction) onAddButtonClicked:(id)sender;
-(IBAction) onFinishButtonClicked:(id)sender;
-(IBAction) onTriangulateButtonClicked:(id)sender;
-(IBAction) onProfileButtonClicked:(id)sender;

@property (nonatomic, assign) int stage;

@property (nonatomic, assign) vector_float2 cursor;

@property (nonatomic, assign) NSUInteger maxVerticesCount;
@property (nonatomic, assign) vector_float2* polygonVerticesData;
@property (nonatomic, assign) NSUInteger currentPolygonVerticesCount;
@property (nonatomic, assign) NSUInteger totalVerticesCount;
@property (nonatomic, strong) NSMutableArray<NSNumber* >* polygonSizes;

@property (nonatomic, assign) bool isCurrentLineValid;
@property (nonatomic, assign) bool isCloseLineValid;

@property (nonatomic, strong) id<MTLBuffer> endLineIndicesBuffer;
@property (nonatomic, strong) id<MTLBuffer> triangleLinesIndicesBuffer;
@property (nonatomic, strong) id<MTLBuffer> trianglesIndicesBuffer;

@end

@implementation ViewController

-(void) updateEndLineIndicesBuffer {
    if (_polygonSizes.count > 0)
    {
        uint32_t* endLineIndices = (uint32_t*) malloc(sizeof(uint32_t) * 2 * _polygonSizes.count);
        int vertexIndex = 0;
        uint32_t* pEndLineIndices = endLineIndices;
        for (NSNumber* polygonSize in _polygonSizes)
        {
            uint32_t verticesCount = (uint32_t)[polygonSize integerValue];
            *pEndLineIndices++ = vertexIndex;
            *pEndLineIndices++ = vertexIndex + verticesCount - 1;
            vertexIndex += verticesCount;
        }
        _endLineIndicesBuffer = [_mtView.device newBufferWithBytes:endLineIndices length:(sizeof(uint32_t) * 2 * _polygonSizes.count) options:MTLResourceOptionCPUCacheModeDefault];
        free(endLineIndices);
    }
    else
    {
        _endLineIndicesBuffer = nil;
    }
}

-(void) setControlStates {
    _addButton.enabled = _stage == 0 && _totalVerticesCount < _maxVerticesCount && _isCurrentLineValid;
    _finishButton.enabled = _stage == 0 && _isCloseLineValid && _currentPolygonVerticesCount > 1;
    
    _triangulateButton.enabled = _polygonSizes.count > 0;
    _fillSwitch.enabled = _triangulateButton.enabled;
    _profileButton.enabled = _triangulateButton.enabled;
    _profileLabel.hidden = !_triangulateButton.enabled;
}

-(void) validateGeometry {
    _isCurrentLineValid = true;
    _isCloseLineValid = true;
    int vertexIndex = 0;
    if (_polygonSizes.count > 0)
    {
        for (NSNumber* polygonSize in _polygonSizes)
        {
            uint32_t verticesCount = (uint32_t)[polygonSize integerValue];
            vertexIndex += verticesCount;
        }
    }
    vector_float2 currentPoint = _polygonVerticesData[vertexIndex + _currentPolygonVerticesCount];
    if (_currentPolygonVerticesCount > 0)
    {
        vector_float2 currentLine[] = {_polygonVerticesData[vertexIndex + _currentPolygonVerticesCount - 1], currentPoint};
        vector_float2 closeLine[] = {_polygonVerticesData[vertexIndex], currentPoint};
        vertexIndex = 0;
        for (NSNumber* polygonSize in _polygonSizes)
        {
            uint32_t verticesCount = (uint32_t)[polygonSize integerValue];
            if (_isCurrentLineValid)
            {
                if (isLinesIntersectWithLineStrip(currentLine[0], currentLine[1], _polygonVerticesData + vertexIndex, verticesCount - 1))
                {
                    _isCurrentLineValid = false;
                }
            }
            if (_isCloseLineValid)
            {
                if (isLinesIntersectWithLineStrip(closeLine[0], closeLine[1], _polygonVerticesData + vertexIndex, verticesCount - 1))
                {
                    _isCloseLineValid = false;
                }
            }
            vertexIndex += verticesCount;
        }
        
        uint32_t* endLineIndices = (uint32_t*)_endLineIndicesBuffer.contents;
        if (_isCurrentLineValid)
        {
            if (isLinesIntersectWithLines(currentLine[0], currentLine[1], _polygonVerticesData, endLineIndices, _polygonSizes.count))
            {
                _isCurrentLineValid = false;
            }
        }
        if (_isCloseLineValid)
        {
            if (isLinesIntersectWithLines(closeLine[0], closeLine[1], _polygonVerticesData, endLineIndices, _polygonSizes.count))
            {
                _isCloseLineValid = false;
            }
        }
            
        if (_currentPolygonVerticesCount > 2)
        {
            if (_isCurrentLineValid)
            {
                if (isLinesIntersectWithLineStrip(currentLine[0], currentLine[1], _polygonVerticesData + vertexIndex, _currentPolygonVerticesCount - 2))
                {
                    _isCurrentLineValid = false;
                }
            }
            if (_isCloseLineValid)
            {
                if (isLinesIntersectWithLineStrip(closeLine[0], closeLine[1], _polygonVerticesData + vertexIndex + 1, _currentPolygonVerticesCount - 2))
                {
                    _isCloseLineValid = false;
                }
            }
        }
    }

    if (_isCurrentLineValid)
    {
        vector_float2* polygonStart = _polygonVerticesData;
        for (NSInteger i = 0; i < _polygonSizes.count; ++i)
        {
            size_t polygonSize = (size_t)[_polygonSizes[i] integerValue];
            if (0 == i)
            {
                if (!isPointInPolygon(currentPoint, polygonStart, polygonSize))
                {
                    _isCurrentLineValid = false;
                    break;
                }
            }
            else
            {
                if (isPointInPolygon(currentPoint, polygonStart, polygonSize))
                {
                    _isCurrentLineValid = false;
                    break;
                }
            }
            polygonStart += polygonSize;
        }
    }

    [self setControlStates];
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
    NSLog(@"mtkView:drawableSizeWillChange:");
}

- (void)drawInMTKView:(nonnull MTKView *)view {
//    NSLog(@"drawInMTKView:");
    id<MTLCommandBuffer> commandBuffer = [_mtCommandQueue commandBuffer];
    [commandBuffer enqueue];
    
    if (!_isCurrentLineValid)
        _mtView.clearColor = MTLClearColorMake(1, 0, 0, 1);
    else if (!_isCloseLineValid)
        _mtView.clearColor = MTLClearColorMake(1.0, 0.5, 0.5, 1);
    else
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

    if (0 == _stage)
    {
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
        {// Draw completed polygons:
            
            for (NSNumber* polygonSize in _polygonSizes)
            {// Draw lines of each polygon except the last line(from 0 to N-1):
                uint32_t verticesCount = (uint32_t)[polygonSize integerValue];
                [renderEncoder drawPrimitives:MTLPrimitiveTypeLineStrip vertexStart:vertexIndex vertexCount:verticesCount instanceCount:1];
                vertexIndex += verticesCount;
            }
            // Draw the 'last' lines:
            
            [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:(2 * _polygonSizes.count) indexType:MTLIndexTypeUInt32 indexBuffer:_endLineIndicesBuffer indexBufferOffset:0];
        }
        if (_currentPolygonVerticesCount > 0)
        {// Draw the incompleted(editing) polygon:
            [renderEncoder drawPrimitives:MTLPrimitiveTypeLineStrip vertexStart:vertexIndex vertexCount:_currentPolygonVerticesCount + 1 instanceCount:1];
            if (_currentPolygonVerticesCount > 1)
            {
                [renderEncoder setFragmentBytes:&whiteColor length:sizeof(vector_float4) atIndex:ColorSlot];
                vector_float2 line[] = {_polygonVerticesData[vertexIndex], _polygonVerticesData[vertexIndex + _currentPolygonVerticesCount]};
                [renderEncoder setVertexBytes:line length:sizeof(line) atIndex:VertexSlot];
                [renderEncoder drawPrimitives:MTLPrimitiveTypeLine vertexStart:0 vertexCount:2];
            }
        }
    }
    else if (1 == _stage && _triangleLinesIndicesBuffer)
    {
        // The number of output triangles produced for a polygon with n points is, (n - 2) + 2*(#holes)
        size_t totalPolygonVertices = _totalVerticesCount - _currentPolygonVerticesCount;
        size_t trianglesCount = totalPolygonVertices - 2 + 2 * (_polygonSizes.count - 1);
        [renderEncoder setVertexBytes:_polygonVerticesData length:sizeof(vector_float2) * totalPolygonVertices atIndex:VertexSlot];
        [renderEncoder setFragmentBytes:&greenColor length:sizeof(vector_float4) atIndex:ColorSlot];
        if (_fillSwitch.isOn)
        {
            [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:(3 * trianglesCount) indexType:MTLIndexTypeUInt32 indexBuffer:_trianglesIndicesBuffer indexBufferOffset:0];
        }
        else
        {
            [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeLine indexCount:(6 * trianglesCount) indexType:MTLIndexTypeUInt32 indexBuffer:_triangleLinesIndicesBuffer indexBufferOffset:0];
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
    
    [self validateGeometry];
}

-(IBAction) onAddButtonClicked:(id)sender {
    switch (_stage)
    {
        case 0:
        {
            _polygonVerticesData[_totalVerticesCount++] = _cursor;
            _polygonVerticesData[_totalVerticesCount] = (vector_float2){0.f, 0.f};
            _currentPolygonVerticesCount++;
            [self validateGeometry];
        }
            break;
            
        default:
            break;
    }
}

-(void) triangulate {
    if (_polygonSizes.count == 0) return;
    int* polygonSizes = (int*) malloc(sizeof(int) * _polygonSizes.count);
    size_t totalPolygonVertices = _totalVerticesCount - _currentPolygonVerticesCount;
    double* vertices = (double*) malloc(sizeof(double) * 2 * (totalPolygonVertices + 1));
    size_t* reorderedIndices = (size_t*) malloc(sizeof(size_t) * totalPolygonVertices);
    // The number of output triangles produced for a polygon with n points is, (n - 2) + 2*(#holes)
    size_t trianglesCount = totalPolygonVertices - 2 + 2 * (_polygonSizes.count - 1);
    int* triangles = (int*) malloc(sizeof(int) * 3 * trianglesCount);
    size_t vertexStartIndex = 0;
    double* pDst = vertices + 2;//vertices[0] must NOT be used (i.e. i/p starts from vertices[1] instead
    size_t* pIndices = reorderedIndices;
    for (NSInteger i = 0; i < _polygonSizes.count; ++i)
    {
        int polygonSize = [_polygonSizes[i] intValue];
        polygonSizes[i] = polygonSize;
        
        bool shouldReverse = isPolygonClockwise(_polygonVerticesData + vertexStartIndex, polygonSize) ^ (0 != i);
        if (shouldReverse)
        {
            const vector_float2* pSrc = _polygonVerticesData + vertexStartIndex + polygonSize - 1;
            size_t originalIndex = vertexStartIndex + polygonSize - 1;
            for (NSInteger j = polygonSize; j > 0; --j)
            {
                pDst[0] = pSrc->x;
                pDst[1] = pSrc->y;
                pDst += 2;
                pSrc--;
                *pIndices++ = originalIndex--;
            }
        }
        else
        {
            const vector_float2* pSrc = _polygonVerticesData + vertexStartIndex;
            size_t originalIndex = vertexStartIndex;
            for (NSInteger j = polygonSize; j > 0; --j)
            {
                pDst[0] = pSrc->x;
                pDst[1] = pSrc->y;
                pDst += 2;
                pSrc++;
                *pIndices++ = originalIndex++;
            }
        }
        
        vertexStartIndex += polygonSize;
    }
    
    SeidelTriangulator* seidel = NULL;
    triangulate_polygon(&seidel, (int)_polygonSizes.count, polygonSizes, (double(*)[2])vertices, (int(*)[3])triangles);
    SeidelTriangulatorRelease(seidel);
    
    uint32_t* triangleLinesIndices = (uint32_t*) malloc(sizeof(uint32_t) * 6 * trianglesCount);
    uint32_t* trianglesIndices = (uint32_t*) malloc(sizeof(uint32_t) * 3 * trianglesCount);
    uint32_t* pTriangleLines = triangleLinesIndices;
    uint32_t* pDstTriangles = trianglesIndices;
    int* pSrcTriangles = triangles;
    for (int i=0; i<trianglesCount; ++i)
    {
        uint32_t v0 = (uint32_t) reorderedIndices[pSrcTriangles[0] - 1];
        uint32_t v1 = (uint32_t) reorderedIndices[pSrcTriangles[1] - 1];
        uint32_t v2 = (uint32_t) reorderedIndices[pSrcTriangles[2] - 1];
        pTriangleLines[0] = v0;
        pTriangleLines[1] = v1;
        pTriangleLines[2] = v1;
        pTriangleLines[3] = v2;
        pTriangleLines[4] = v2;
        pTriangleLines[5] = v0;
        pDstTriangles[0] = v0;
        pDstTriangles[1] = v1;
        pDstTriangles[2] = v2;
        pSrcTriangles += 3;
        pTriangleLines += 6;
        pDstTriangles += 3;
    }
    _triangleLinesIndicesBuffer = [_mtView.device newBufferWithBytes:triangleLinesIndices length:(sizeof(uint32_t) * 6 * trianglesCount) options:MTLResourceOptionCPUCacheModeDefault];
    _trianglesIndicesBuffer = [_mtView.device newBufferWithBytes:trianglesIndices length:(sizeof(uint32_t) * 3 * trianglesCount) options:MTLResourceOptionCPUCacheModeDefault];
    free(triangleLinesIndices);
    free(trianglesIndices);
    
    free(polygonSizes);
    free(vertices);
    free(reorderedIndices);
    free(triangles);
}

-(void) profileTriangulation {
    if (_polygonSizes.count == 0) return;
    int* polygonSizes = (int*) malloc(sizeof(int) * _polygonSizes.count);
    size_t totalPolygonVertices = _totalVerticesCount - _currentPolygonVerticesCount;
    double* vertices = (double*) malloc(sizeof(double) * 2 * (totalPolygonVertices + 1));
    size_t* reorderedIndices = (size_t*) malloc(sizeof(size_t) * totalPolygonVertices);
    // The number of output triangles produced for a polygon with n points is, (n - 2) + 2*(#holes)
    size_t trianglesCount = totalPolygonVertices - 2 + 2 * (_polygonSizes.count - 1);
    int* triangles = (int*) malloc(sizeof(int) * 3 * trianglesCount);
    size_t vertexStartIndex = 0;
    double* pDst = vertices + 2;//vertices[0] must NOT be used (i.e. i/p starts from vertices[1] instead
    size_t* pIndices = reorderedIndices;
    for (NSInteger i = 0; i < _polygonSizes.count; ++i)
    {
        int polygonSize = [_polygonSizes[i] intValue];
        polygonSizes[i] = polygonSize;
        
        bool shouldReverse = isPolygonClockwise(_polygonVerticesData + vertexStartIndex, polygonSize) ^ (0 != i);
        if (shouldReverse)
        {
            const vector_float2* pSrc = _polygonVerticesData + vertexStartIndex + polygonSize - 1;
            size_t originalIndex = vertexStartIndex + polygonSize - 1;
            for (NSInteger j = polygonSize; j > 0; --j)
            {
                pDst[0] = pSrc->x;
                pDst[1] = pSrc->y;
                pDst += 2;
                pSrc--;
                *pIndices++ = originalIndex--;
            }
        }
        else
        {
            const vector_float2* pSrc = _polygonVerticesData + vertexStartIndex;
            size_t originalIndex = vertexStartIndex;
            for (NSInteger j = polygonSize; j > 0; --j)
            {
                pDst[0] = pSrc->x;
                pDst[1] = pSrc->y;
                pDst += 2;
                pSrc++;
                *pIndices++ = originalIndex++;
            }
        }
        
        vertexStartIndex += polygonSize;
    }
    
    const int TestCount = 1024;
    NSDate* startTime = [NSDate date];
    SeidelTriangulator* seidel = NULL;
    for (int i=TestCount; i>0; --i)
    {
        triangulate_polygon(&seidel, (int)_polygonSizes.count, polygonSizes, (double(*)[2])vertices, (int(*)[3])triangles);
    }
    SeidelTriangulatorRelease(seidel);
    NSTimeInterval timeUsage = [[NSDate date] timeIntervalSinceDate:startTime];
    dispatch_async(dispatch_get_main_queue(), ^{
        self.profileLabel.text = [NSString stringWithFormat:@"%ld vertices, %ld holes, total %f ms for %d times, average %f ms for one triangulation", totalPolygonVertices, self.polygonSizes.count - 1, timeUsage * 1000, TestCount, timeUsage * 1000 / TestCount];
        self.stage = 0;
        [self setControlStates];
    });
    
    free(polygonSizes);
    free(vertices);
    free(reorderedIndices);
    free(triangles);
}

-(IBAction) onProfileButtonClicked:(id)sender {
    _stage = 2;
    _profileButton.enabled = NO;
    _profileLabel.text = @"Profiling...";
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self profileTriangulation];
    });
    [self setControlStates];
}

-(IBAction) onFinishButtonClicked:(id)sender {
    switch (_stage)
    {
        case 0:
        {
            _totalVerticesCount++;
            [_polygonSizes addObject:@(_currentPolygonVerticesCount + 1)];
            _currentPolygonVerticesCount = 0;
            _finishButton.enabled = NO;
            _triangulateButton.enabled = _polygonSizes.count <= 0;
            _fillSwitch.enabled = _triangulateButton.enabled;
            
            [self updateEndLineIndicesBuffer];
            [self validateGeometry];
        }
            break;
            
        default:
            break;
    }
}

-(IBAction) onTriangulateButtonClicked:(id)sender {
    switch (_stage)
    {
    case 0:
        _stage = 1;
        [self triangulate];
        [_triangulateButton setTitle:@"继续编辑" forState:UIControlStateNormal];
        [self setControlStates];
        break;
    case 1:
        _stage = 0;
        _totalVerticesCount -= _currentPolygonVerticesCount;
        _currentPolygonVerticesCount = 0;
        [_triangulateButton setTitle:@"三角化" forState:UIControlStateNormal];
        [self setControlStates];
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
    _isCurrentLineValid = true;
    _isCloseLineValid = true;
    [self setControlStates];
    
    UIPanGestureRecognizer* panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPanGestureRecognized:)];
    [_mtView addGestureRecognizer:panRecognizer];
}


@end
