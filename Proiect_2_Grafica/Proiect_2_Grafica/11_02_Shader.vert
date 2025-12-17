#version 330 core

// --- LAYOUT NOU ---
// Loc 0: Pozitie (Toata lumea are asta)
// Loc 1: Normala (Pt Iarba) SAU Culoare (Pt Copaci/Cer) - Aici e conflictul pe care il rezolvam
// Loc 2: UV (Pt Iarba) SAU Normala (Pt Copaci - dar o ignoram momentan)
// Loc 3: Tangenta (Pt Iarba)

layout(location=0) in vec3 in_Position;
layout(location=1) in vec3 in_Normal_Or_Color; // <--- Variabila hibrida
layout(location=2) in vec2 in_TexCoord;
layout(location=3) in vec3 in_Tangent;

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
    // ==========================================
    // CAZ 5: IARBA (NORMAL MAPPING)
    // ==========================================
    if (codCol == 5)
    {
        gl_Position = projection * view * model * vec4(in_Position, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position, 1.0));
        ex_ViewPos = viewPos;
        ex_TexCoord = in_TexCoord;
        
        // Aici in_Normal_Or_Color este chiar Normala
        vec3 N = normalize(vec3(model * vec4(in_Normal_Or_Color, 0.0)));
        vec3 T = normalize(vec3(model * vec4(in_Tangent, 0.0)));
        T = normalize(T - dot(T, N) * N);
        vec3 B = cross(N, T);
        ex_TBN = mat3(T, B, N);
    }

    // ==========================================
    // CAZ 0: COPACI (Gradient + Volum)
    // ==========================================
    if (codCol == 0)
    {
        gl_Position = projection * view * model * vec4(in_Position, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position, 1.0));
        ex_ViewPos = viewPos;

        // Culoarea originala a copacului
        vec3 vertexColor = in_Normal_Or_Color; 

        // --- RESTAURARE EFECT LUMINA ---
        
        // 1. Gradient Vertical (Fake Ambient Occlusion)
        // Baza copacului e mai intunecata (mai aproape de sol/umbra), varful e mai luminat.
        // Impartim Y la 15.0f pentru o tranzitie mai fina decat inainte.
        float heightLight = 0.5 + 0.5 * (in_Position.y / 15.0);
        
        // Limitam valorile sa nu fie nici prea negre, nici prea arse
        heightLight = clamp(heightLight, 0.4, 1.1); 

        // 2. Simulare Soare (Fake Diffuse)
        // Calculam o lumina simpla presupunand ca toate frunzele privesc in SUS (0,1,0)
        // Asta face ca copacii sa reactioneze putin cand muti soarele
        vec3 lightDir = normalize(lightPos - ex_FragPos);
        float sunLight = max(dot(vec3(0,1,0), lightDir), 0.0) * 0.4; // 40% influenta soare

        // Combinam: Culoare * (Gradient + Soare)
        vec3 finalColor = vertexColor * (heightLight + sunLight);
        
        ex_Color = vec4(finalColor, 1.0);
    }

    // ==========================================
    // CAZ 1: UMBRE
    // ==========================================
    if (codCol == 1)
    {
        vec4 rotatedPos = model * vec4(in_Position, 1.0);
        vec4 shadowWorldPos = matrUmbra * rotatedPos;
        shadowWorldPos.y += 0.05;
        gl_Position = projection * view * shadowWorldPos;
        ex_Color = vec4(0.1, 0.1, 0.1, 0.5);
    }

    // ==========================================
    // CAZ 2: SKYBOX (Cer)
    // ==========================================
    if (codCol == 2)
    {
        gl_Position = projection * view * model * vec4(in_Position, 1.0);
        
        // Skybox-ul trimite gradientul tot pe canalul 1 via SceneObjects.cpp
        // Deci il luam direct de aici
        ex_Color = vec4(in_Normal_Or_Color, 1.0);
    }

    // ==========================================
    // CAZ 3, 4: UI, SOARE
    // ==========================================
    if (codCol == 3 || codCol == 4) 
    {
        gl_Position = projection * view * model * vec4(in_Position, 1.0);
        ex_FragPos = vec3(model * vec4(in_Position, 1.0));
        ex_LocalPos = in_Position; 
        
        if (codCol == 3) ex_Color = overrideColor;
        else ex_Color = vec4(1.0); 
    }
}