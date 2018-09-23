#version 330

out vec4 fragColor;
in VsData
{
	vec2 texCoordOut;
} vsData;

uniform sampler2D normalTexture;
uniform sampler2D noiseTexture;
uniform sampler2D depthTexture;

uniform vec3 kernel[64];
uniform mat4 projection;
uniform mat4 projectionToViewMatrix; //Inverse projection matrix

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(1920.0/4.0, 1080.0/4.0); // screen = 1280x720

vec3 decodeLocation(vec2 texCoord) {
  vec4 clipSpaceLocation;
  clipSpaceLocation.xy = texCoord.xy * 2.0f - 1.0f;
  clipSpaceLocation.z = texture(depthTexture, texCoord.xy).r * 2.0f - 1.0f;
  clipSpaceLocation.w = 1.0f;
  vec4 homogenousLocation = projectionToViewMatrix * clipSpaceLocation;
  return homogenousLocation.xyz / homogenousLocation.w;
}

void main()
{
	int kernelSize = 64;
	float radius   = 5.0;
	float bias     = 0.05;
    vec3 fragPos   = decodeLocation(vsData.texCoordOut);
	
	if(fragPos.x != 0.0 && fragPos.y != 0.0 && fragPos.z != 0.0){
		vec3 normal    = texture(normalTexture, vsData.texCoordOut).rgb;
		vec3 randomVec = texture(noiseTexture, vsData.texCoordOut * noiseScale).xyz;  

		vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
		vec3 bitangent = cross(normal, tangent);
		mat3 TBN       = mat3(tangent, bitangent, normal); 

		float occlusion = 0.0;
		for(int i = 0; i < kernelSize; i = i + int(radius))
		{
			// get sample position
			vec3 sample = TBN * kernel[i]; // From tangent to view-space
			sample = fragPos + sample * radius; 

			vec4 offset = vec4(sample, 1.0);
			offset      = projection * offset;    // from view to clip-space
			offset.xyz /= offset.w;               // perspective divide
			offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0 
		
			float sampleDepth = decodeLocation(offset.xy).z;
			float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
			occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;   
		}  
		occlusion = 1.0 - (occlusion / kernelSize);
		fragColor.r = occlusion;  
	}
}