#ifndef INSTANCING
$input v_texcoord0, v_posTime
#endif

#include <bgfx_shader.sh>

#ifndef INSTANCING
  #include <newb/main.sh>

  SAMPLER2D_AUTOREG(s_SkyTexture);
#endif

void main() {
  #ifndef INSTANCING
    vec3 viewDir = normalize(v_posTime.xyz);
    vec4 diffuse = texture2D(s_SkyTexture, v_texcoord0);

    vec3 color = renderEndSky(getEndHorizonCol(), getEndZenithCol(), normalize(v_posTime.xyz), v_posTime.w);
    color += 2.8*diffuse.rgb; // stars

    vec4 bh = renderBlackhole(viewDir, v_posTime.w);
    color *= bh.a;
    color += bh.rgb;

    color = colorCorrection(color);

    gl_FragColor = vec4(color, 1.0);
  #else
    gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
  #endif
}
