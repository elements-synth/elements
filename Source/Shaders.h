/*
  ==============================================================================
    Shaders.h
    Elements - PBR GLSL Shaders (Cook-Torrance + Environment + Refraction + SSS)
  ==============================================================================
*/

#pragma once

// Vertex shader: transforms position/normal to world space
static constexpr const char* pbrVertexShader = R"(
    attribute vec4 a_position;
    attribute vec3 a_normal;

    uniform mat4 u_modelMatrix;
    uniform mat4 u_viewMatrix;
    uniform mat4 u_projMatrix;
    uniform mat3 u_normalMatrix;

    varying vec3 v_worldPos;
    varying vec3 v_worldNormal;

    void main()
    {
        vec4 worldPos = u_modelMatrix * a_position;
        v_worldPos = worldPos.xyz;
        v_worldNormal = normalize(u_normalMatrix * a_normal);
        gl_Position = u_projMatrix * u_viewMatrix * worldPos;
    }
)";

// Fragment shader: Cook-Torrance PBR + Environment Cubemap + Refraction + SSS
static constexpr const char* pbrFragmentShader = R"(
    varying vec3 v_worldPos;
    varying vec3 v_worldNormal;

    // Existing PBR uniforms
    uniform vec3 u_albedo;
    uniform float u_metallic;
    uniform float u_roughness;

    uniform vec3 u_lightPos[3];
    uniform vec3 u_lightColor[3];
    uniform int u_lightEnabled[3];

    uniform vec3 u_cameraPos;

    // New material uniforms
    uniform float u_ior;
    uniform float u_transparency;
    uniform float u_sssStrength;
    uniform float u_sssRadius;
    uniform vec3 u_absorptionColor;

    // Environment map (equirectangular HDR, sampled as 2D texture)
    uniform sampler2D u_envMap;

    const float PI = 3.14159265359;

    // =========================================================================
    // Equirectangular environment sampling
    // =========================================================================

    vec3 sampleEnvMap(vec3 dir)
    {
        // Convert direction to equirectangular UV
        // u = atan(z, x) / (2*PI) + 0.5
        // v = asin(y) / PI + 0.5
        float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
        float v = asin(clamp(dir.y, -1.0, 1.0)) / PI + 0.5;
        return texture2D(u_envMap, vec2(u, v)).rgb;
    }

    // =========================================================================
    // PBR Functions (Cook-Torrance)
    // =========================================================================

    float distributionGGX(vec3 N, vec3 H, float roughness)
    {
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float NdotH2 = NdotH * NdotH;
        float denom = NdotH2 * (a2 - 1.0) + 1.0;
        denom = PI * denom * denom;
        return a2 / max(denom, 0.0001);
    }

    float geometrySchlickGGX(float NdotV, float roughness)
    {
        float r = roughness + 1.0;
        float k = (r * r) / 8.0;
        return NdotV / (NdotV * (1.0 - k) + k);
    }

    float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
    {
        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
    }

    vec3 fresnelSchlick(float cosTheta, vec3 F0)
    {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
    {
        vec3 maxR = vec3(1.0 - roughness);
        return F0 + (max(maxR, F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    void main()
    {
        vec3 N = normalize(v_worldNormal);
        vec3 V = normalize(u_cameraPos - v_worldPos);
        float NdotV = max(dot(N, V), 0.0);

        // (a) F0 from IOR: F0 = ((1-n)/(1+n))^2
        float iorRatio = (1.0 - u_ior) / (1.0 + u_ior);
        float F0scalar = iorRatio * iorRatio;
        vec3 F0 = mix(vec3(F0scalar), u_albedo, u_metallic);

        // =====================================================================
        // Direct lighting (Cook-Torrance with 3 point lights)
        // =====================================================================
        vec3 Lo = vec3(0.0);

        // Also accumulate SSS contribution from lights
        vec3 sssAccum = vec3(0.0);

        for (int i = 0; i < 3; ++i)
        {
            if (u_lightEnabled[i] == 0)
                continue;

            vec3 L = normalize(u_lightPos[i] - v_worldPos);
            vec3 H = normalize(V + L);

            float distance = length(u_lightPos[i] - v_worldPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = u_lightColor[i] * attenuation * 12.0;

            // Cook-Torrance BRDF
            float D = distributionGGX(N, H, u_roughness);
            float G = geometrySmith(N, V, L, u_roughness);
            vec3  F = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = D * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;

            vec3 kS = F;
            vec3 kD = (1.0 - kS) * (1.0 - u_metallic);

            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * u_albedo / PI + specular) * radiance * NdotL;

            // (d) Subsurface scattering contribution
            if (u_sssStrength > 0.0)
            {
                // Wrap lighting: extends diffuse to wrap around the surface
                float wrapAngle = u_sssRadius * 0.5;
                float wrapDiffuse = max(0.0, (dot(N, L) + wrapAngle) / (1.0 + wrapAngle));
                vec3 sssWrap = u_absorptionColor * wrapDiffuse;

                // Translucency: view-dependent, simulates light passing through
                float translucency = pow(clamp(dot(V, -L), 0.0, 1.0), 3.0) * 0.5;
                vec3 sssTrans = u_absorptionColor * translucency;

                sssAccum += (sssWrap + sssTrans) * radiance * u_sssStrength;
            }
        }

        // =====================================================================
        // (b) Environment reflection from cubemap
        // =====================================================================
        vec3 R = reflect(-V, N);
        vec3 envReflection = sampleEnvMap(R);

        // Approximate roughness blur: blend with average cubemap color
        // (no mipmaps in GL2, so we approximate)
        vec3 envAverage = vec3(0.45, 0.42, 0.40);  // Average of our studio HDRI
        envReflection = mix(envReflection, envAverage, u_roughness * u_roughness);

        // Fresnel-weighted environment reflection
        vec3 F_env = fresnelSchlickRoughness(NdotV, F0, u_roughness);
        vec3 envSpec = envReflection * F_env;

        // =====================================================================
        // (c) Refraction (only if transparent)
        // =====================================================================
        vec3 refractionColor = vec3(0.0);

        if (u_transparency > 0.0)
        {
            float eta = 1.0 / max(u_ior, 0.01);
            vec3 refractDir = refract(-V, N, eta);

            // If total internal reflection, fall back to reflection
            if (length(refractDir) < 0.001)
                refractDir = R;

            vec3 refractSample = sampleEnvMap(refractDir);

            // Beer's law absorption: exp(-thickness * (1 - absorptionColor) * density)
            float thickness = 1.0;  // Approximate uniform thickness
            vec3 absorption = exp(-thickness * (vec3(1.0) - u_absorptionColor) * 2.0);
            refractionColor = refractSample * absorption;
        }

        // =====================================================================
        // (e) Final composition
        // =====================================================================

        // Ambient
        vec3 ambient = vec3(0.03) * u_albedo;

        // Environment reflection strength: reduced for opaque metals, stronger for transparent/glossy
        float envStrength = mix(0.15, 0.4, u_transparency) * mix(1.0, 0.5, u_metallic);

        // Opaque color: ambient + direct lighting + environment specular + SSS
        vec3 opaqueColor = ambient + Lo + envSpec * envStrength + sssAccum;

        // Transparent color: refraction + env + SSS
        vec3 transColor = refractionColor + envSpec * 0.3 + sssAccum;

        // Blend based on transparency
        vec3 color = mix(opaqueColor, transColor, u_transparency);

        // HDR tonemapping (Reinhard) + gamma correction
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));

        // Alpha: slightly transparent for refractive materials
        float alpha = 1.0 - u_transparency * 0.3;

        gl_FragColor = vec4(color, alpha);
    }
)";
