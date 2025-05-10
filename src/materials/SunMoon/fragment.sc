$input v_texcoord0, v_pos

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/config.h>
  #include <newb/main.sh>
  

  uniform vec4 SunMoonColor;
  uniform vec4 ViewPositionAndTime;
  uniform vec4 FogColor;
  
  SAMPLER2D_AUTOREG(s_SunMoonTexture);
#endif

void main() {
  #ifndef INSTANCING
    vec4 color = vec4_splat(0.0);
    float t = 0.6*ViewPositionAndTime.w;

    float c = atan2(v_pos.x, v_pos.z);
    float g = 1.0-min(length(v_pos*2.0), 1.0);
    g *= g*g*g;
    //g *= 1.2+0.25*sin(c*2.0 - t)*sin(c*5.0 + t);
    //g *= 0.5;
    
  float day = pow(max(min(1.0 - FogColor.r * 1.2, 1.0), 0.0), 0.4);
  float night = pow(max(min(1.0 - FogColor.r * 1.5, 1.0), 0.0), 1.2);
  float dusk = max(FogColor.r - FogColor.b, 0.0);
  float tr = 1.0 - SunMoonColor.a;
  g *= 1.0-0.5*tr*tr*tr;
    
    
    vec2 uv = v_texcoord0;
    ivec2 ts = textureSize(s_SunMoonTexture, 0);
    bool isMoon = ts.x > ts.y;
    if (isMoon) {
      uv = vec2(0.25,0.5)*(floor(uv*vec2(4.0,2.0)) + 0.5 + 10.0*v_pos.xz);
      g *= max(0.0, 1.0)* night;
      vec3 precol = vec3(0.0,0.0,0.3);
      vec3 nightcol = vec3(0.18,0.20,0.4);
      nightcol *= max(0.0, 1.0)*night;
      color.rgb += g*mix(precol,nightcol, night);
    } else {
      g = mix(g, mix(g, 0.0,(FogColor.r-FogColor.b, 0.0)), dusk);
      uv = 0.5 + 10.0*v_pos.xz;
      vec3 daycol =  vec3(0.3,0.2,0.0);
      vec3 dawncol = vec3(0.4,0.3,0.0);
      daycol *= max(0.5, 1.0)*day;
      dawncol *= max(0.5,1.0)*dusk;
      color.rgb += g*mix(daycol,dawncol, dusk);
    }

    if (max(abs(v_pos.x),abs(v_pos.z)) < 0.5/10.0) {
      color += texture2D(s_SunMoonTexture, uv);
    }

    color.rgb *= SunMoonColor.rgb;
    color.rgb *= 4.5*color.rgb;
    
    color.a = 1.0 - 0.5*tr*tr*tr;
    
    color.rgb = colorCorrection(color.rgb);
    gl_FragColor = color;
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
