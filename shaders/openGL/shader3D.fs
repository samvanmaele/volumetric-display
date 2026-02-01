#version 450 core

const float PI = 3.14159265358979323846;
const float realRes = 570.5;
const uint WIDTH  = 570;
const uint HEIGHT = 1140;
const uint DEPTH  = 160;
const uint STRIDE = WIDTH * HEIGHT;
const uint WORDS_PER_Z = (DEPTH + 31) / 32;
const uint  NUM_BITS = 24u;

layout(std430, binding = 0) readonly buffer VolumeBuffer {uint data[];} volume;
uniform float helixAngle;
uniform float deltaAngle;

out vec4 colour;

uint sampleZ(vec2 pos, float angle)
{
    float a = atan(pos.y, pos.x) + angle;
    float dist = fract(a /  PI) * float(DEPTH);
    return min(uint(dist), DEPTH - 1u);
}

void main()
{
    uint x = uint(gl_FragCoord.x);
    uint y = uint(gl_FragCoord.y);
    uint baseIndex = x + y * WIDTH;

    float offset = 0.5 * float(y & 1u);
    vec2 newPos  = vec2(float(x) + offset, floor(float(y) * 0.5) + offset);
    vec2 centre = vec2(realRes * 0.5);
    newPos = newPos - centre;

    uint packedVal = 0u;
    for (uint i = 0u; i < NUM_BITS; ++i)
    {
        float angle = helixAngle + float(i) * deltaAngle;
        uint z = sampleZ(newPos, angle);

        uint bitIdx  = z & 31u;
        uint wz      = z >> 5;
        uint wordIdx = baseIndex + (wz * STRIDE);

        uint b = (volume.data[wordIdx] >> bitIdx) & 1u;
        packedVal |= b << i;
    }

    uvec3 rgb;
    rgb.r =  packedVal        & 0xFFu;
    rgb.g = (packedVal >> 8)  & 0xFFu;
    rgb.b = (packedVal >> 16) & 0xFFu;

    colour = vec4(vec3(rgb) / 255.0, 1.0);
}