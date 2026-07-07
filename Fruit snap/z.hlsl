// Uniform inputs from script (Animation Controller)
uniform float u_RestorationProgress; // 0.0 (Fully Degraded) to 1.0 (Fully Clean)
uniform sampler2D u_NoiseMap;
uniform sampler2D u_MetalBaseTex;
uniform sampler2D u_DegradedTex;

void fragment() {
    // Read textures
    vec4 cleanColor = texture(u_MetalBaseTex, v_uv);
    vec4 degradedColor = texture(u_DegradedTex, v_uv);
    float noise = texture(u_NoiseMap, v_uv).r;

    // Create an organic threshold transition using the noise map
    // Progress bar acts as a cutoff for the noise threshold
    float transitionMask = smoothstep(u_RestorationProgress - 0.1, u_RestorationProgress + 0.1, noise);

    // Lerp Albedo, Metallic, and Roughness based on the mask
    vec3 finalAlbedo = mix(degradedColor.rgb, cleanColor.rgb, transitionMask);
    float finalMetallic = mix(degradedMetallic, cleanMetallic, transitionMask);
    float finalRoughness = mix(degradedRoughness, cleanRoughness, transitionMask);

    // Output to PBR engine
    ALBEDO = finalAlbedo;
    METALLIC = finalMetallic;
    ROUGHNESS = finalRoughness;
}
