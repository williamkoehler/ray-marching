struct VertexInputType
{
	float4 position : POSITION;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 screenPos : SCREEN;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;

	input.position.w = 1.0f;
	output.position = input.position;

	output.screenPos = input.position.xy;

	return output;
}