#version 430

// Water + Terrain shader   
// 10/2014 Created by Frank Hugenroth /frankenburgh/
// 'hash' and 'noise' function by iq
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// V1.1  - added 'real' water height (hits terrain) and waterheight is visible on shadow
//
// CLICK and MOVE the MOUSE to:
// X -> Change water height  /  Y -> Change water clarity.

// some parameters....

float coast2water_fadedepth = 0.10;
float large_waveheight      = 0.50; // change to adjust the "heavy" waves
float large_wavesize        = 4.;  // factor to adjust the large wave size
float small_waveheight      = .6;  // change to adjust the small random waves
float small_wavesize        = .5;   // factor to ajust the small wave size
float water_softlight_fact  = 15.;  // range [1..200] (should be << smaller than glossy-fact)
float water_glossylight_fact= 120.; // range [1..200]
float particle_amount       = 70.;
vec3 watercolor             = vec3(0.43, 0.60, 0.66); // 'transparent' low-water color (RGB)
vec3 watercolor2            = vec3(0.06, 0.07, 0.11); // deep-water color (RGB, should be darker than the low-water color)
vec3 water_specularcolor    = vec3(1.3, 1.3, 0.9);    // specular Color (RGB) of the water-highlights
vec3 light;

uniform sampler2D noiseTexture;
uniform float time;
uniform mat4 normal;     // Normal matrix

in VsData
{
	vec2 texCoordOut;
	vec3 positionOut;
} vsData;

layout(location = 0) out vec4 out_1;
layout(location = 1) out vec4 out_2;
layout(location = 2) out vec4 out_3;


// calculate random value
float hash( float n )
{
    return fract(sin(n)*43758.5453123);
}

// 2d noise function
float noise1( in vec2 x )
{
  vec2 p  = floor(x);
  vec2 f  = smoothstep(0.0, 1.0, fract(x));
  float n = p.x + p.y*57.0;
  return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
    mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y);
}

float noise(vec2 p)
{
  return textureLod(noiseTexture,p*vec2(1./256.),0.0).x;
}

float height_map( vec2 p )
{
#if USETEXTUREHEIGHT
  float f = 0.15+textureLod(iChannel2, p*0.6, 0.0).r*2.;
#else
  mat2 m = mat2( 0.9563*1.4,  -0.2924*1.4,  0.2924*1.4,  0.9563*1.4 );
  p = p*6.;
  float f = 0.6000*noise1( p ); p = m*p*1.1;
  f += 0.2500*noise1( p ); p = m*p*1.32;
  f += 0.1666*noise1( p ); p = m*p*1.11;
  f += 0.0834*noise( p ); p = m*p*1.12;
  f += 0.0634*noise( p ); p = m*p*1.13;
  f += 0.0444*noise( p ); p = m*p*1.14;
  f += 0.0274*noise( p ); p = m*p*1.15;
  f += 0.0134*noise( p ); p = m*p*1.16;
  f += 0.0104*noise( p ); p = m*p*1.17;
  f += 0.0084*noise( p );
  const float FLAT_LEVEL = 0.525;
  if (f<FLAT_LEVEL)
      f = f;
  else
      f = pow((f-FLAT_LEVEL)/(1.-FLAT_LEVEL), 2.)*(1.-FLAT_LEVEL)*2.0+FLAT_LEVEL; // makes a smooth coast-increase
#endif
  return clamp(f, 0., 10.);
}

const mat2 m = mat2( 0.72, -1.60,  1.60,  0.72 );

float water_map( vec2 p, float height )
{
  vec2 p2 = p*large_wavesize;
  vec2 shift1 = 0.001*vec2( time*160.0*2.0, time*120.0*2.0 );
  vec2 shift2 = 0.001*vec2( time*190.0*2.0, -time*130.0*2.0 );

  // coarse crossing 'ocean' waves...
  float f = 0.6000*noise( p );
  f += 0.2500*noise( p*m );
  f += 0.1666*noise( p*m*m );
  float wave = sin(p2.x*0.622+p2.y*0.622+shift2.x*4.269)*large_waveheight*f*height*height ;

  p *= small_wavesize;
  f = 0.;
  float amp = 1.0, s = .5;
  for (int i=0; i<9; i++)
  { p = m*p*.947; f -= amp*abs(sin((noise( p+shift1*s )-.5)*2.)); amp = amp*.59; s*=-1.329; }
 
  return wave+f*small_waveheight;
}

float nautic(vec2 p)
{
  p *= 18.;
  float f = 0.;
  float amp = 1.0, s = .5;
  for (int i=0; i<3; i++)
  { p = m*p*1.2; f += amp*abs(smoothstep(0., 1., noise( p+time*s ))-.5); amp = amp*.5; s*=-1.227; }
  return pow(1.-f, 5.);
}

float particles(vec2 p)
{
  p *= 200.;
  float f = 0.;
  float amp = 1.0, s = 1.5;
  for (int i=0; i<3; i++)
  { p = m*p*1.2; f += amp*noise( p+time*s ); amp = amp*.5; s*=-1.227; }
  return pow(f*.35, 7.)*particle_amount;
}


void main()
{
	light = vec3(0.0, 0.0, 1.5); // position of the sun
    //light = vec3(-0., sin(time*0.5)*.5 + .35, 2.8); // position of the sun
	vec2 uv = (vsData.texCoordOut - vec2(-0.12, +0.25));

    float WATER_LEVEL = 0.94; // Water level (range: 0.0 - 2.0)
    
    float height = height_map( uv );
    vec3 col;
    
    float waveheight = clamp(WATER_LEVEL*3.-1.5, 0., 1.);
    float level = WATER_LEVEL + .2*water_map(uv*15. + vec2(time*.1), waveheight);
    //if (height > level)
    //{
    //    col = CalcTerrain(uv, height);
    //}
    //if (height <= level)
    //{
        vec2 dif = vec2(.0, .01);
        vec2 pos = uv*15. + vec2(time*.01);
        float h1 = water_map(pos-dif,waveheight);
        float h2 = water_map(pos+dif,waveheight);
        float h3 = water_map(pos-dif.yx,waveheight);
        float h4 = water_map(pos+dif.yx,waveheight);
        vec3 normwater = normalize(vec3(h3-h4, h1-h2, .125)); // norm-vector of the 'bumpy' water-plane
		
		
		uv += normwater.xy*.002*(level-height);

        float coastfade = 1.0;//clamp((level-height)/coast2water_fadedepth, 0., 1.);
        float coastfade2= 1.0;//clamp((level-height)/deepwater_fadedepth, 0., 1.);
        float intensity = col.r*.2126+col.g*.7152+col.b*.0722;
        watercolor = mix(watercolor*intensity, watercolor2, smoothstep(0., 1., coastfade2));

        vec3 r0 = vec3(uv, WATER_LEVEL);
        vec3 rd = normalize( light - r0 ); // ray-direction to the light from water-position
        float grad     = dot(normwater, rd); // dot-product of norm-vector and light-direction
        float specular = pow(grad, water_softlight_fact);  // used for soft highlights                          
        float specular2= pow(grad, water_glossylight_fact); // used for glossy highlights
        float gradpos  = dot(vec3(0., 0., 1.), rd);
        float specular1= smoothstep(0., 1., pow(gradpos, 5.));  // used for diffusity (some darker corona around light's specular reflections...)                          
        float watershade  = 1.0; //test against shadow here
        watercolor *= 2.2+watershade;
   		watercolor += (.2+.8*watershade) * ((grad-1.0)*.5+specular) * .25;
   		watercolor /= (1.+specular1*1.25);
   		watercolor += watershade*specular2*water_specularcolor;
        watercolor += watershade*coastfade*(1.-coastfade2)*(vec3(.5, .6, .7)*nautic(uv)+vec3(1., 1., 1.)*particles(uv));
        
        col = mix(col, watercolor, coastfade);
    //}
    
	//switch z and y normal
	normwater = vec3((normal * vec4(normwater, 0.0)).xyz); //Transform normal coordinate in with the normal matrix
        
	out_1 = vec4(col , 1.0);
	out_2 = vec4(normalize(normwater), 1.0);
	out_3 = vec4(vsData.positionOut.xyz, 1.0);
}
