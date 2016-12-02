/*
	Deferred Lighting 计算光照
*/

struct VS_INPUT
{
	float3 Position : POSITION;
};

struct VS_OUTPUT
{
	float4 Position : POSITION;
	float4 Texcoord : TEXCOORD;
};

float4x4 wvp_matrix;
float4x4 texspace_matrix;

texture worldposTexture;
sampler2D worldposSampler = sampler_state
{
	texture = <worldposTexture>;
	MinFilter = Point;
	MagFilter = Point;
	MipFilter = Point;   
	AddressU  = Clamp;
	AddressV  = Clamp;
};

texture worldnormalTexture;
sampler2D worldnormalSampler = sampler_state
{
	texture = <worldnormalTexture>;
	MinFilter = Point;
	MagFilter = Point;
	MipFilter = Point;   
	AddressU  = Clamp;
	AddressV  = Clamp;
};

texture diffuseTexture;
sampler2D diffuseSampler = sampler_state
{
	texture = <diffuseTexture>;
	MinFilter = Point;
	MagFilter = Point;
	MipFilter = Point;   
	AddressU  = Clamp;
	AddressV  = Clamp;
};

//
// Vertex Shader
//
VS_OUTPUT VS(VS_INPUT In)
{
	VS_OUTPUT Out;
	
	Out.Position = mul(float4(In.Position.xyz, 1.0f), wvp_matrix);

	// 把屏幕坐标系对应成贴图坐标，它只是简单地平移加上缩放
	Out.Texcoord = mul(Out.Position, texspace_matrix);
	
	return Out;
}

float4 vCameraPos;
float4 vLightPos;
float4 vLightColor;

// 计算点光源的 Pixel Shader
float4 PS_PointLight(VS_OUTPUT In) : COLOR
{
	float2 vTexcoord = In.Texcoord.xy / In.Texcoord.w; // perspective correct
	
	// 从贴图中取出这个像素的 position, normal, diffuse
	float3 vWorldPos = tex2D(worldposSampler, vTexcoord).xyz;
	float3 vWorldNormal = tex2D(worldnormalSampler, vTexcoord).xyz;
	float4 vAlbedo = tex2D(diffuseSampler, vTexcoord);
	
	// 计算光照
	float3 vLightDir = normalize(vLightPos - vWorldPos);
	float3 vCameraDir = normalize(vCameraPos - vWorldPos);
	float3 vHalfVector = normalize(vCameraDir + vLightDir);
	
	float  fDistance = distance(vLightPos, vWorldPos);
	float  fAttenuation = 1.0f-fDistance/vLightPos.w;
	
	float4 vDiffuse = vLightColor * saturate(dot(vLightDir, vWorldNormal));
	float4 vSpecular = vLightColor * pow(saturate(dot(vHalfVector, vWorldNormal)), 30.0f) * vAlbedo.a;
	float4 vLighting = (vDiffuse + vSpecular) * vAlbedo * fAttenuation;
			
	return vLighting;
}

// 计算 Ambient Light 的 Pixel Shader
float4 PS_AmbientLight(VS_OUTPUT In) : COLOR
{
	float2 texcoord = In.Texcoord.xy / In.Texcoord.w;
	float4 DiffuseTex = tex2D(diffuseSampler, texcoord);
	float4 vDiffuse = vLightColor * DiffuseTex;
			
	return vDiffuse;
}

technique PointLight
{
	pass p0 
	{
		VertexShader = compile vs_2_0 VS();
		PixelShader = compile ps_2_0 PS_PointLight();

		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ONE;

		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
	}
}

technique AmbientLight
{
	pass p0 
	{
		VertexShader = compile vs_2_0 VS();
		PixelShader = compile ps_2_0 PS_AmbientLight();

		ALPHABLENDENABLE = FALSE;
		ZENABLE = FALSE;
		ZWRITEENABLE = FALSE;
	}
}

struct VS_INPUT_VC
{
	float4 Position : POSITION;
	float4 Color : COLOR;
};

struct VS_OUTPUT_VC
{
	float4 Position : POSITION;
	float4 Color : COLOR;
};

VS_OUTPUT_VC VS_VC(VS_INPUT_VC In)
{
	VS_OUTPUT_VC Out;
	
	Out.Position = mul(float4(In.Position.xyz, 1.0f), wvp_matrix);
	Out.Color = In.Color;
	
	return Out;
}

VS_OUTPUT_VC VS_V(float4 Position : POSITION)
{
	VS_OUTPUT_VC Out;
	
	Out.Position = mul(float4(Position.xyz, 1.0f), wvp_matrix);
	Out.Color = float4(1,1,1,1);
	
	return Out;
}

float4 PS_COLOR(VS_OUTPUT_VC In) : COLOR
{
	return In.Color;
}

technique DisplayLight
{
	pass p0 
	{
		VertexShader = compile vs_2_0 VS_VC();
		PixelShader = compile ps_2_0 PS_COLOR();
		ALPHABLENDENABLE = FALSE;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
	}
}

technique DisplaySphere
{
	pass p0 
	{
		VertexShader = compile vs_2_0 VS_V();
		PixelShader = compile ps_2_0 PS_COLOR();
		ALPHABLENDENABLE = FALSE;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
	}
}
