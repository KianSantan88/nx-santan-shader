#ifndef SKY_H
#define SKY_H

#include "detection.h"
#include "noise.h"

struct nl_skycolor {
  vec3 zenith;
  vec3 horizon;
  vec3 horizonEdge;
};

// rainbow spectrum
vec3 spectrum(float x) {
    vec3 s = vec3(x-0.5, x, x+0.5);
    s = smoothstep(1.0,0.0,abs(s));
    return s*s;
}

vec3 getUnderwaterCol(vec3 FOG_COLOR) {
  return 2.0*NL_UNDERWATER_TINT*FOG_COLOR*FOG_COLOR;
}

vec3 getEndZenithCol() {
  return NL_END_ZENITH_COL;
}

vec3 getEndHorizonCol() {
  return NL_END_HORIZON_COL;
}

// values used for getting sky colors
vec3 getSkyFactors(vec3 FOG_COLOR) {
  vec3 factors = vec3(
    max(FOG_COLOR.r*0.6, max(FOG_COLOR.g, FOG_COLOR.b)), // intensity val
    1.5*max(FOG_COLOR.r-FOG_COLOR.b, 0.0), // viewing sun
    min(FOG_COLOR.g, 0.26) // rain brightness
  );

  factors.z *= factors.z;

  return factors;
}

vec3 getZenithCol(float rainFactor, vec3 FOG_COLOR, vec3 fs) {
  vec3 zenithCol = NL_NIGHT_ZENITH_COL*(1.0-FOG_COLOR.b);
  zenithCol += NL_DAWN_ZENITH_COL*((0.7*fs.x*fs.x) + (0.4*fs.x) + fs.y);
  zenithCol = mix(zenithCol, (0.7*fs.x*fs.x + 0.3*fs.x)*NL_DAY_ZENITH_COL, fs.x*fs.x);
  zenithCol = mix(zenithCol*(1.0+0.5*rainFactor), NL_RAIN_ZENITH_COL*fs.z*13.2, rainFactor);

  return zenithCol;
}

vec3 getHorizonCol(float rainFactor, vec3 FOG_COLOR, vec3 fs) {
  vec3 horizonCol = NL_NIGHT_HORIZON_COL*(1.0-FOG_COLOR.b); 
  horizonCol += NL_DAWN_HORIZON_COL*(((0.7*fs.x*fs.x) + (0.3*fs.x) + fs.y)*1.9); 
  horizonCol = mix(horizonCol, 2.0*fs.x*NL_DAY_HORIZON_COL, fs.x*fs.x);
  horizonCol = mix(horizonCol, NL_RAIN_HORIZON_COL*fs.z*19.6, rainFactor);

  return horizonCol;
}

// tinting on horizon col

vec3 getHorizonEdgeCol(vec3 horizonCol, float rainFactor, vec3 FOG_COLOR) {
  float val = 2.1*(1.1-FOG_COLOR.b)*FOG_COLOR.g*(1.0-rainFactor);
  horizonCol *= vec3_splat(1.0-val) + NL_DAWN_EDGE_COL*val;

  return horizonCol;
}

// 1D sky with three color gradient
vec3 renderOverworldSky(nl_skycolor skycol, vec3 viewDir) {
  float h = 1.0-viewDir.y*viewDir.y;
  float hsq = h*h;
  if (viewDir.y < 0.0) {
    hsq = 0.4 + 0.6*hsq*hsq;
  }

  // gradient 1  h^16
  // gradient 2  h^8 mix h^2
  float gradient1 = hsq*hsq;
  gradient1 *= gradient1;
  float gradient2 = 0.6*gradient1 + 0.4*hsq;
  gradient1 *= gradient1;

  vec3 sky = mix(skycol.horizon, skycol.horizonEdge, gradient1);
  sky = mix(skycol.zenith, skycol.horizon, gradient2);

  return sky;
}

// sunrise/sunset bloom
vec3 getSunBloom(float viewDirX, vec3 horizonEdgeCol, vec3 FOG_COLOR) {
  float factor = FOG_COLOR.r/(0.01 + length(FOG_COLOR));
  factor *= factor;
  factor *= factor;

  float spread = smoothstep(0.0, 1.0, abs(viewDirX));
  float sunBloom = spread*spread;
  sunBloom = 1.0*spread + sunBloom*sunBloom*sunBloom*2.0;

  return NL_MORNING_SUN_COL*horizonEdgeCol*(sunBloom*factor*factor);
}

// custom end sky
// Function to blend colors in the flares, smoothly alternating between two colors
    vec3 getFlareColor(float angle, float t) {
    // Smoother and more gradual oscillation in the color change of the flares
    float mixFactor = 0.5 + 0.5 * sin(angle * 8.0 + t * 0.6);
    return mix(FLARE_COLOR_1, FLARE_COLOR_2, mixFactor);
}

    vec3 renderEndSky(vec3 horizonCol, vec3 zenithCol, vec3 v, float t) {
    vec3 sky = vec3(0.0, 0.0, 0.0);
    v.y = smoothstep(-1.0, 1.6, abs(v.y)); // Smoother transition between the horizon and the zenith

    float a = atan2(v.x, v.z); // Calculate the angle for the flares

    // Adjustments to flare behavior to make them more defined
    float s = sin(a * 2.0 + t * 0.8); // Higher frequency for thinner, straighter flares
    s = pow(abs(s), 2.0); // Use pow to make the flares sharper and taller
    s *= 0.4 + 0.7 * sin(a * 10.0 - 0.3 * t); // Amplitude adjustment for greater height and variation
    float g = smoothstep(1.2 - s, -2.0, v.y); // Adjustment to control the flare height

    // Sky color blending based on position
    float f = (2.0 * g + 1.0 * smoothstep(1.0, -0.1, v.y)); // Adjustment to compensate for change in g
    float h = (1.6 * g + 1.3 * smoothstep(0.9, -0.2, v.y)); // Adjustment to compensate for change in g

    // Mix the sky color
    sky += mix(zenithCol, horizonCol, f * f * f);

    // Get the color of the flares
    vec3 flareColor = getFlareColor(a, t);

    // Apply the flare color with more intensity and definition
    sky += (g * g * 1.5 + 3.5 * pow(h, 4.0)) * flareColor; // Adjustment for more prominent and defined flares

    return sky;
}

// black hole
  vec4 renderBlackhole(vec3 vdir, float t) {
  t *= NL_BH_SPEED;

  float r = 1.8;
  r += 0.0001*t;
  vec3 vr = vdir;
  // manual calculation
  float cx = cos(r);
  float sx = sin(r);
  vr.xy = vec2(cx * vr.x - sx * vr.y, sx * vr.x + cx * vr.y);
  //vr.xy = mat2(cos(r), -sin(r), sin(r), cos(r)) * vr.xy;
  //r *= 2.0;
  //vr.yz = mat2(cos(r), -sin(r), sin(r), cos(r)) * vr.yz;

  vec3 vd = vr-vec3(0.0, -1.0, 0.0);
  float nl = sin(15.0*vd.x + t)*sin(15.0*vd.y - t)*sin(15.0*vd.z + t);
  float a = atan2(vd.x, vd.z);

  float d = NL_BH_DIST*length(vd + 0.003*nl);
  //d *= 1.2 + 0.8*sin(0.2*t);
  float d0 = (0.6-d)/0.6;
  float dm0 = 1.0-max(d0, 0.0);

  float gl = 1.0-clamp(-0.3*d0, 0.0, 1.0);
  float gla = pow(1.0-min(abs(d0), 1.0), 8.0);
  float gl8 = pow(gl, 8.0);

  float hole = 0.9*pow(dm0, 32.0) + 0.1*pow(dm0, 3.0);
  float bh = (gla + 0.8*gl8 + 0.2*gl8*gl8) * hole;

  float df = sin(3.0*a - 4.0*d + 24.0*pow(1.4-d, 4.0) + t);
  df *= 0.9 + 0.1*sin(8.0*a + d + 4.0*t - 4.0*df);
  bh *= 1.0 + pow(df, 4.0)*hole*max(1.0-bh, 0.0);

  vec3 col = bh*4.0*mix(NL_BH_COL_LOW, NL_BH_COL_HIGH , min(bh, 1.0));
  
  return vec4(col, hole);
}

vec3 nlRenderSky(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 FOG_COLOR, float t) {
  vec3 sky;
  viewDir.y = -viewDir.y;

  if (env.end) {
    sky = renderEndSky(skycol.horizon, skycol.zenith, viewDir, t);
  } else {
    sky = renderOverworldSky(skycol, viewDir);
    #ifdef NL_RAINBOW
      sky += mix(NL_RAINBOW_CLEAR, NL_RAINBOW_RAIN, env.rainFactor)*spectrum((viewDir.z+0.6)*8.0)*max(viewDir.y, 0.0)*FOG_COLOR.g;
    #endif
    #ifdef NL_UNDERWATER_STREAKS
      if (env.underwater) {
        float a = atan2(viewDir.x, viewDir.z);
        float grad = 0.5 + 0.5*viewDir.y;
        grad *= grad;
        float spread = (0.5 + 0.5*sin(3.0*a + 0.2*t + 2.0*sin(5.0*a - 0.4*t)));
        spread *= (0.5 + 0.5*sin(3.0*a - sin(0.5*t)))*grad;
        spread += (1.0-spread)*grad;
        float streaks = spread*spread;
        streaks *= streaks;
        streaks = (spread + 3.0*grad*grad + 4.0*streaks*streaks);
        sky += 2.0*streaks*skycol.horizon;
      } else 
    #endif
    if (!env.nether) {
      sky += getSunBloom(viewDir.x, skycol.horizonEdge, FOG_COLOR);
    }
  }

  return sky;
}

// sky reflection on plane
vec3 getSkyRefl(nl_skycolor skycol, nl_environment env, vec3 viewDir, vec3 FOG_COLOR, float t) {
  vec3 refl = nlRenderSky(skycol, env, viewDir, FOG_COLOR, t);

  if (!(env.underwater || env.nether)) {
    float specular = smoothstep(0.7, 0.0, abs(viewDir.z));
    specular *= specular*viewDir.x;
    specular *= specular;
    specular += specular*specular*specular*specular;
    specular *= max(FOG_COLOR.r-FOG_COLOR.b, 0.0);
    refl += 5.0 * skycol.horizonEdge * specular * specular;
  }

  return refl;
}

// shooting star
vec3 nlRenderShootingStar(vec3 viewDir, vec3 FOG_COLOR, float t) {
  // transition vars
  float h = t / (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD);
  float h0 = floor(h);
  t = (NL_SHOOTING_STAR_DELAY + NL_SHOOTING_STAR_PERIOD) * (h-h0);
  t = min(t/NL_SHOOTING_STAR_PERIOD, 1.0);
  float t0 = t*t;
  float t1 = 1.0-t0;
  t1 *= t1; t1 *= t1; t1 *= t1;

  // randomize size, rotation, add motion, add skew
  float r = fract(sin(h0) * 43758.545313);
  float a = 6.2831*r;
  float cosa = cos(a);
  float sina = sin(a);
  vec2 uv = viewDir.xz * (6.0 + 4.0*r);
  uv = vec2(cosa*uv.x + sina*uv.y, -sina*uv.x + cosa*uv.y);
  uv.x += t1 - t;
  uv.x -= 2.0*r + 3.5;
  uv.y += viewDir.y * 3.0;

  // draw star
  float g = 1.0-min(abs((uv.x-0.95))*15.0, 1.0); // source glow
  float s = 1.0-min(abs(8.0*uv.y), 1.0); // line
  s *= s*s*smoothstep(-1.0+1.96*t1, 0.98-t, uv.x); // decay tail
  s *= s*s*smoothstep(1.0, 0.98-t0, uv.x); // decay source
  s *= 1.0-t1; // fade in
  s *= 1.0-t0; // fade out
  s *= 0.8 + 18.0*g*g;
  s *= max(1.0-FOG_COLOR.r-FOG_COLOR.g-FOG_COLOR.b, 0.0); // fade out during day
  return s*vec3(0.2, 0.1, 1.0);
}

// custom galaxy
vec3 nlRenderGalaxy(vec3 vdir, vec3 fogColor, nl_environment env, float t) {
  if (env.underwater) {
    return vec3_splat(0.0);
  }

  t *= NL_GALAXY_SPEED;

  // rotate space
  float cosb = sin(0.2*t);
  float sinb = cos(0.2*t);
  vdir.xy = mul(mat2(cosb, sinb, -sinb, cosb), vdir.xy);

  // noise
  float n0 = 0.5 + 0.5*sin(5.0*vdir.x)*sin(5.0*vdir.y - 0.5*t)*sin(5.0*vdir.z + 0.5*t);
  float n1 = noise3D(15.0*vdir + sin(0.85*t + 1.3));
  float n2 = noise3D(50.0*vdir + 1.0*n1 + sin(0.7*t + 1.0));
  float n3 = noise3D(200.0*vdir - 10.0*sin(0.4*t + 0.500));

  // stars (now single color)
  n3 = smoothstep(0.04, 0.3, n3 + 0.02*n2);
  float gd = vdir.x + 0.1*vdir.y + 0.1*sin(10.0*vdir.z + 0.2*t);
  float st = n1 * n2 * n3 * n3 * (1.0 + 70.0 * gd * gd);
  st = (1.0 - st) / (1.0 + 100.0 * st);
  vec3 stars = vec3(0.2, 0.1, 1.0) * st; // <Stars color over here

  stars *= mix(1.0, NL_GALAXY_DAY_VISIBILITY, env.dayFactor);

  return stars*(1.0-env.rainFactor);
}

nl_skycolor nlUnderwaterSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  s.zenith = getUnderwaterCol(FOG_COLOR);
  s.horizon = s.zenith;
  s.horizonEdge = s.zenith;
  return s;
}

nl_skycolor nlEndSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  s.zenith = getEndZenithCol();
  s.horizon = getEndHorizonCol();
  s.horizonEdge = s.horizon;
  return s;
}

nl_skycolor nlOverworldSkyColors(float rainFactor, vec3 FOG_COLOR) {
  nl_skycolor s;
  vec3 fs = getSkyFactors(FOG_COLOR);
  s.zenith = getZenithCol(rainFactor, FOG_COLOR, fs);
  s.horizon = getHorizonCol(rainFactor, FOG_COLOR, fs);
  s.horizonEdge = getHorizonEdgeCol(s.horizon, rainFactor, FOG_COLOR);
  return s;
}

nl_skycolor nlSkyColors(nl_environment env, vec3 FOG_COLOR) {
  nl_skycolor s;
  if (env.underwater) {
    s = nlUnderwaterSkyColors(env.rainFactor, FOG_COLOR);
  } else if (env.end) {
    s = nlEndSkyColors(env.rainFactor, FOG_COLOR);
  } else {
    s = nlOverworldSkyColors(env.rainFactor, FOG_COLOR);
  }
  return s;
}

#endif
