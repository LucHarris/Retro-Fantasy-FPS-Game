//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

struct MaterialData
{
	float4   DiffuseAlbedo;
	float3   FresnelR0;
	float    Roughness;
	float4x4 MatTransform;
	uint     DiffuseMapIndex;
	uint     MatPad0;
	uint     MatPad1;
	uint     MatPad2;
};

struct Shockwave
{
    // origin
    float3 Pos;
    // from pos
    float Raduis;
    // from radius
    float Width;
    // on normal
    float Strength;
    // radius multiplier
    float Speed;
    // speed and strength
    float Drag;
};

// An array of textures, which is only supported in shader model 5.1+.  Unlike Texture2DArray, the textures
// in this array can be different sizes and formats, making it more flexible than texture arrays.
Texture2D gDiffuseMap[4] : register(t0);

// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t3 in space0. 
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
	uint gObjPad0;
	uint gObjPad1;
	uint gObjPad2;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
    Shockwave gShockwaves[1];
    float gTimeLimit;
    float gTimeLeft;
    float gPadding[86];
};

// typical input
//struct VertexIn
//{
//    float3 PosL : POSITION;
//    float3 NormalL : NORMAL;
//    float2 TexC : TEXCOORD;
//};

//// input for points to be exanded
struct VertexIn
{
    float3 Pos : POSITION;
    float2 Size : SIZE;
    float4 TexRect : TEXRECT;
    int BillboardType : BILLBOARD;

};

struct VertexOut
{
    float3 Pos : POSITION;
    float2 Size : SIZE;
    float4 TexRect : TEXRECT;
    // only accetps floats
    float BillboardType : BILLBOARD;
    float2 Buffer0 : B0;
};

struct GeomOut
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexC    : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    
    vout.Pos = vin.Pos;
    vout.Size = vin.Size;
    vout.TexRect = vin.TexRect;
    // offset to avoid rounding errors
    vout.BillboardType = float(vin.BillboardType) + 0.5f; 
    return vout;
}

void BillboardingSingleFixed(VertexOut gin, inout TriangleStream<GeomOut> triStream)
{
    float x = gin.Size.x * 0.5f;
    float y = gin.Size.y * 0.5f;
    
    float3 v[4];
	v[0] = gin.Pos + float3(-x,-y,0.0f) ;
	v[1] = gin.Pos + float3(-x,+y,0.0f) ;
	v[2] = gin.Pos + float3(+x,-y,0.0f) ;
	v[3] = gin.Pos + float3(+x,+y,0.0f) ;
    
    
    float2 uv[4];
    uv[0] = float2(gin.TexRect.x , gin.TexRect.y + gin.TexRect.w );                 //float2(1.0f,1.0f);
    uv[1] = float2(gin.TexRect.x , gin.TexRect.y );                                 //float2(1.0f,0.0f);
    uv[2] = float2(gin.TexRect.x + gin.TexRect.z, gin.TexRect.y + gin.TexRect.w);   //float2(0.0f,1.0f);
    uv[3] = float2(gin.TexRect.x + gin.TexRect.z,gin.TexRect.y);                    //float2(0.0f,0.0f); 
    
    GeomOut output = gin;
    for(int i = 0; i < 4; ++i)
    {
        output.PosH = mul(float4(v[i],1.0f), gViewProj);
        output.PosW = v[i].xyz;
        output.NormalW = float3(0.0f, 0.0f, -1.0f);
        
        output.TexC = uv[i];
        triStream.Append(output);
    }
}

void BillboardingPointOrientation(VertexOut gin, inout TriangleStream<GeomOut> triStream)
{
    // todo
}

void BillboardingAxisOrientation(VertexOut gin, inout TriangleStream<GeomOut> triStream)
{
    //Set Up Vectors
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin.Pos;
	look.y = 0.0f;
	look = normalize(look);
	float3 right = cross(up, look);

	//Half Height/Width
	float halfWidth = 0.5f * gin.Size.x;
	float halfHeight = 0.5f * gin.Size.y;

	//Calculate Quad
	float4 v[4];
	v[0] = float4(gin.Pos + halfWidth * right - halfHeight * up, 1.0f);
	v[1] = float4(gin.Pos + halfWidth * right + halfHeight * up, 1.0f);
	v[2] = float4(gin.Pos - halfWidth * right - halfHeight * up, 1.0f);
	v[3] = float4(gin.Pos - halfWidth * right + halfHeight * up, 1.0f);

    float2 uv[4];
    uv[0] = float2(gin.TexRect.x , gin.TexRect.y + gin.TexRect.w );                 //float2(1.0f,1.0f);
    uv[1] = float2(gin.TexRect.x , gin.TexRect.y );                                 //float2(1.0f,0.0f);
    uv[2] = float2(gin.TexRect.x + gin.TexRect.z, gin.TexRect.y + gin.TexRect.w);   //float2(0.0f,1.0f);
    uv[3] = float2(gin.TexRect.x + gin.TexRect.z,gin.TexRect.y);                    //float2(0.0f,0.0f); 
    
    GeomOut output = gin;
    for(int i = 0; i < 4; ++i)
    {
        output.PosH = mul(float4(v[i]), gViewProj);
        output.PosW = v[i].xyz;
        output.NormalW = look;
        output.TexC = uv[i];
        triStream.Append(output);
    }
}
void BillboardingFixedDouble(VertexOut gin, inout TriangleStream<GeomOut> triStream)
{
    // todo
}
void BillboardingFixedCross(VertexOut gin, inout TriangleStream<GeomOut> triStream)
{
    // todo
}

[maxvertexcount(4)]
void GS(point VertexOut gin[1], 
        uint primID : SV_PrimitiveID, 
        inout TriangleStream<GeomOut> triStream)
{
    // BillboardType POINT_ORIENTATION
    if (gin[0].BillboardType < 1.0f)
    {
        BillboardingPointOrientation(gin[0], triStream);
    } 
    // BillboardType AXIS_ORIENTATION
    else if(gin[0].BillboardType < 2.0f)
    {
        BillboardingAxisOrientation(gin[0], triStream);
    } 
    // BillboardType FIXED_SINGLE
    else if(gin[0].BillboardType < 3.0f)
    {
        BillboardingSingleFixed(gin[0], triStream);
    }
    // BillboardType FIXED_DOUBLE
    else if(gin[0].BillboardType < 4.0f)
    {
        BillboardingFixedDouble(gin[0], triStream);
    }
    // BillboardType FIXED_CROSS
    else if(gin[0].BillboardType < 5.0f)
    {
        BillboardingFixedCross(gin[0], triStream);
    }
    else // BillboardType NONE
    {
        // does not display
    }
}


float4 PS(GeomOut pin) : SV_Target
{
	// Fetch the material data.
	MaterialData matData = gMaterialData[gMaterialIndex];
	float4 diffuseAlbedo = matData.DiffuseAlbedo;
	float3 fresnelR0 = matData.FresnelR0;
	float  roughness = matData.Roughness;
	uint diffuseTexIndex = matData.DiffuseMapIndex;

	// Dynamically look up the texture in the array.
	diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamPointWrap, pin.TexC);
    
    clip(diffuseAlbedo.a - 0.5f);

    
    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    // Light terms.
    float4 ambient = gAmbientLight*diffuseAlbedo;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    litColor = min(litColor, diffuseAlbedo);
    // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a;
    
    // todo remove
    [branch]
    switch (1)
    {
        case 0:
            litColor.a = 0.0f;
            break;
    }
    
    
    
    return litColor;
}

