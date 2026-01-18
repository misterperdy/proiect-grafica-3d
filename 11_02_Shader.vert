#version 330 core

// Format vertex unificat pentru toate mesh-urile:
// layout(location=0) vec4 pozitie
// layout(location=1) vec4 culoare
// layout(location=2) vec3 normala
// (location 3 rezervat pentru viitor)

layout(location=0) in vec4 in_Position;
layout(location=1) in vec4 in_Color;
layout(location=2) in vec3 in_Normal;

out vec4 gl_Position; 
out vec4 ex_Color;
out vec3 ex_FragPos;
out vec3 ex_ViewPos;
out vec3 ex_LocalPos; 
out vec2 ex_TexCoord;
out mat3 ex_TBN;

uniform mat4 matrUmbra;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform int codCol;
uniform vec4 overrideColor;

void main(void)
{
    // Ini?ializãm valorile transmise cãtre fragment shader cu valori sigure,
    // ca sã evitãm interpolãri nedefinite între modurile `codCol`.
    ex_TexCoord = vec2(0.0);
    ex_LocalPos = vec3(0.0);
    ex_TBN = mat3(1.0);
    ex_Color = in_Color;
    ex_FragPos = vec3(model * vec4(in_Position.xyz, 1.0));
    ex_ViewPos = viewPos;

    // ==========================================
    // CAZ 5: mod pãstrat doar pentru compatibilitate (nu mai este folosit în scenã)
    // ==========================================
    if (codCol == 5)
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);
    }

    // ==========================================
    // CAZ 0: obiecte iluminate (copaci + teren cu culoare pe vârfuri)
    // Calculeazã iluminare simplã (ambient + difuz + specular) direct în vertex.
    // ==========================================
    if (codCol == 0)
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position.xyz, 1.0));
        ex_ViewPos = viewPos;
        ex_LocalPos = in_Position.xyz;

        // Normalã stabilã pentru fragment shader (corect transformatã în spa?iul world)
        vec3 N = normalize(mat3(model) * in_Normal);
        ex_TBN = mat3(1.0);
        ex_TBN[2] = N;

        vec3 baseColor = in_Color.rgb;

        vec3 L = normalize(lightPos - ex_FragPos);
        vec3 V = normalize(viewPos - ex_FragPos);
        vec3 H = normalize(L + V);

        float ambientStrength = 0.33;
        vec3 ambient = ambientStrength * baseColor;

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * baseColor * lightColor;

        float spec = pow(max(dot(N, H), 0.0), 36.0);
        vec3 specular = 0.08 * spec * lightColor;

        // Micã „ancorare” pe verticalã (reduce impresia cã trunchiurile plutesc)
        float heightAO = clamp(0.90 + 0.10 * (in_Position.y / 20.0), 0.80, 1.05);

        vec3 finalColor = (ambient + diffuse + specular) * heightAO;
        ex_Color = vec4(finalColor, in_Color.a);
    }

    // ==========================================
    // CAZ 1: umbre (proiec?ie planarã)
    // Mutã pozi?ia în spa?iul „umbre” ?i aplicã un bias în func?ie de pantã.
    // ==========================================
    if (codCol == 1)
    {
        vec4 worldPos = model * vec4(in_Position.xyz, 1.0);

        vec3 N = normalize(mat3(model) * in_Normal);
        vec3 L = normalize(lightPos - worldPos.xyz);

        // Bias dependent de unghi (ajutã când „soarele” e aproape de orizont)
        float ndotl = abs(dot(N, L));
        float bias = mix(0.25, 0.02, ndotl);

        vec4 shadowWorldPos = matrUmbra * worldPos;
        shadowWorldPos.y += bias;

        gl_Position = projection * view * shadowWorldPos;
        ex_Color = vec4(0.07, 0.05, 0.045, 0.28);
    }

    // ==========================================
    // CAZ 2: skybox (cer)
    // ==========================================
    if (codCol == 2)
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);
        ex_Color = vec4(in_Color.rgb, 1.0);
    }

    // ==========================================
    // CAZ 3, 4: UI / soare-lunã
    // CAZ 3 folose?te `overrideColor`, CAZ 4 pãstreazã culoarea din mesh.
    // ==========================================
    if (codCol == 3 || codCol == 4) 
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position.xyz, 1.0));
        ex_LocalPos = in_Position.xyz;

        if (codCol == 3) ex_Color = overrideColor;
        else ex_Color = in_Color;
    }

    // ==========================================
    // CAZ 6: billboard texturat (flipbook foc)
    // Deriveazã UV din coordonatele locale XY ale planului.
    // ==========================================
    if (codCol == 6)
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);

        // UV din limitele locale. Planul e în XY, dar dimensiunea poate varia la export.
        vec2 uv = in_Position.xy;
        uv = (uv - vec2(-1.0)) / vec2(2.0); // fallback implicit

        // Dacã planul e centrat în 0, transformãm [-s..s] -> [0..1]
        float sx = max(abs(in_Position.x), 0.0001);
        float sy = max(abs(in_Position.y), 0.0001);
        uv = vec2(in_Position.x / (2.0 * sx) + 0.5, in_Position.y / (2.0 * sy) + 0.5);

        ex_TexCoord = clamp(uv, 0.0, 1.0);
        ex_Color = in_Color;
    }

    // ==========================================
    // CAZ 7: obiecte texturate simple (bazã foc/tabãrã)
    // Mapare planã UV din coordonate world XZ.
    // ==========================================
    if (codCol == 7)
    {
        vec4 worldPos = model * vec4(in_Position.xyz, 1.0);
        gl_Position = projection * view * worldPos;
        ex_FragPos = worldPos.xyz;
        ex_ViewPos = viewPos;

        // Mapare planã simplã (scala se ajusteazã pentru densitatea texturii)
        ex_TexCoord = worldPos.xz * 0.08;

        ex_Color = in_Color;
    }

    // ==========================================
    // CAZ 8: glow procedural (aurã)
    // Are nevoie de UV pentru gradient.
    // ==========================================
    if (codCol == 8)
    {
        gl_Position = projection * view * model * vec4(in_Position.xyz, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position.xyz, 1.0));
        ex_ViewPos = viewPos;

        // Mapare planã pentru geometrie tip disc (razã ~0.5): [-0.5..0.5] -> [0..1]
        ex_TexCoord = in_Position.xy + 0.5;

        ex_Color = in_Color;
    }
}