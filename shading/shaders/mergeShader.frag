#version 330

out vec4 fragColor;
in vec2 texCoord;

uniform sampler2D deferredTexture;  
uniform sampler2D bloomTexture;   

void main(){
    
	
	const float gamma = 2.2;
    vec3 hdrColor = texture(deferredTexture, texCoord).rgb;      
    vec3 bloomColor = texture(bloomTexture, texCoord).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    //vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    //result = pow(result, vec3(1.0 / gamma));
    fragColor = vec4(hdrColor, 1.0);
}