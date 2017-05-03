#ifndef MME_HOOK_SAMPLE
#error MME hook sample plugin required
#endif
int kind : MMEHookSample_KIND = 0;
float4 color : MY_SEMANTIC;
float4x4 WorldViewProjMatrix : WORLDVIEWPROJECTION;

void SampleVS(
	in float4 Position : POSITION,
	out float4 oPosition : POSITION) {
	oPosition = mul(Position, WorldViewProjMatrix);
}

float4 SamplePS(in float2 coord : TEXCOORD0) : COLOR {
	return color;
}

technique MainTec0 {
	pass CopyDummyScreenTex < string Script= "Draw=Buffer;"; > {
		VertexShader = compile vs_3_0 SampleVS();
		PixelShader  = compile ps_3_0 SamplePS();
	}
}
