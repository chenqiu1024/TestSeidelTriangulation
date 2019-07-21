//
//  Shaders.metal
//  TestTriangulation
//
//  Created by Dom Chiu on 2019/7/21.
//  Copyright © 2019 Dom Chiu. All rights reserved.
//

#include <metal_stdlib>
#include "ShaderDefines.h"

using namespace metal;

vertex float4 TrivialVertex2DFunction(constant float2* vertices [[buffer(VertexSlot)]]
                                      , constant float2& viewsize [[buffer(ViewportSlot)]]
                                      , uint vertexID [[vertex_id]]) {
//    return float4((vertices[vertexID] / viewsize - 0.5f) * 2.f, 0.f, 1.f);
    return float4(vertices[vertexID], 0.f, 1.f);
}

fragment half4 TrivialFragmentFunction(constant float4& color [[buffer(ColorSlot)]]) {
//    return half4(1.f, 0.f, 0.f, 1.f);
    return half4(color);
}
