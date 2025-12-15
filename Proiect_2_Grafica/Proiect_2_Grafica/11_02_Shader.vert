// ================================================
// | Grafica pe calculator                        |
// ================================================
// | Laboratorul XI - 11_02_Shader.vert |
// ======================================
// 
//  Shaderul de varfuri / Vertex shader - afecteaza geometria scenei; 

#version 330 core

layout(location=0) in vec4 in_Position;
layout(location=1) in vec4 in_Color;
layout(location=2) in vec3 in_Normal;

out vec4 gl_Position; 
out vec4 ex_Color;
out vec3 ex_FragPos;   // Trimitem pozitia pixelului la Fragment Shader
out vec3 ex_ViewPos;   // Trimitem pozitia camerei la Fragment Shader

uniform mat4 matrUmbra;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform int codCol;

// Variabile uniforme pentru ceata
//uniform float fogDensity; // Densitate
uniform float fogStart;
uniform float fogEnd;
uniform vec3 fogColor;    // Culoare

void main(void)
  {
  	if ( codCol==0 )
    {
        // 1. Aplicam matricea MODEL (rotatia)
        gl_Position = projection * view * model * in_Position;
        
        // 2. Recalculam pozitia si normala in functie de rotatie
        vec3 FragPos = vec3(model * in_Position); 
        vec3 Normal = mat3(model) * in_Normal; 

        ex_FragPos = FragPos;
        ex_ViewPos = viewPos;

        vec3 inLightPos = lightPos;
        vec3 inViewPos = viewPos;

        // --- ILUMINARE ---
        float ambientStrength = 0.2f;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(inLightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = pow(diff, 0.2) * lightColor;
        
        float specularStrength = 0.5f;
        vec3 viewDir = normalize(inViewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1);
        vec3 specular = specularStrength * spec * lightColor;  
        
        vec3 baseColorRGB = in_Color.rgb;
        float baseAlpha = in_Color.a;

        vec3 lightingColor = (ambient + diffuse) * baseColorRGB + specular; 

        // --- CEATA --- mutata in frag shader
        //float dist = distance(inViewPos, FragPos);
        //float f = (fogEnd - dist) / (fogEnd - fogStart);
        //f = clamp(f, 0.0, 1.0);

        //vec3 finalRGB = mix(fogColor, lightingColor, f);
        //ex_Color = vec4(finalRGB, baseAlpha);
        ex_Color = vec4(lightingColor, baseAlpha);
    }

    // UMBRA
    if ( codCol==1 )
    {
        vec4 rotatedPos = model * in_Position;

        // aplicam matricea de umbra pe pozitia rotita
        vec4 shadowWorldPos = matrUmbra * rotatedPos;
        shadowWorldPos.y += 0.1; // pt combatere z fighting
        
        //calculam pozitia pe ecran
        gl_Position = projection * view * shadowWorldPos;

        // Calculam CEATA pentru umbra (folosind pozitia umbrei) MUTATA IN FRAG SHDAER
        //float dist = distance(viewPos, vec3(shadowWorldPos));
        //float f = (fogEnd - dist) / (fogEnd - fogStart);
        //f = clamp(f, 0.0, 1.0);

        //vec3 shadowBaseColor = vec3(0.0, 0.0, 0.0);
        //vec3 finalShadowColor = mix(fogColor, shadowBaseColor, f);

        //ex_Color = vec4(finalShadowColor, 1.0);

        ex_FragPos = shadowWorldPos.xyz / shadowWorldPos.w;
        ex_ViewPos = viewPos;

        // Culoarea umbrei e neagra
        ex_Color = vec4(0.1, 0.1, 0.15, 0.5);
    }
   } 
 