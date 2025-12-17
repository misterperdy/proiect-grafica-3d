#version 330 core

in vec4 ex_Color;
in vec3 ex_FragPos;
in vec3 ex_ViewPos;
in vec3 ex_LocalPos;
in vec2 ex_TexCoord;
in mat3 ex_TBN;

out vec4 out_Color;

uniform int codCol;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 lightPos;

// TEXTURI
uniform sampler2D diffuseMap; 
uniform sampler2D normalMap;

void main(void)
{
    // ------------------------------------
    // CAZ 5: IARBA (Boosted Normal Map)
    // ------------------------------------
    if (codCol == 5)
    {
        vec3 color = texture(diffuseMap, ex_TexCoord).rgb;
        
        // Extragem normala
        vec3 normal = texture(normalMap, ex_TexCoord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(ex_TBN * normal);

        // 1. AMBIENT: Scadem intensitatea (de la 0.5 la 0.2)
        // Asta face ca santurile dintre firele de iarba sa fie mai intunecate.
        vec3 ambient = 0.2 * color; 
        
        // 2. DIFFUSE: Crestem putin intensitatea soarelui (x 1.2)
        vec3 lightDir = normalize(lightPos - ex_FragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * color * 1.2; 

        // 3. SPECULAR (Nou): Adaugam stralucire!
        // Iarba reflecta lumina cand e "proaspata" sau uda.
        // Asta va evidentia enorm denivelarile din Normal Map.
        vec3 viewDir = normalize(ex_ViewPos - ex_FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        
        // 32.0 este "shininess" (cat de concentrat e punctul de lumina)
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = vec3(0.3) * spec; // 0.3 este culoarea stralucirii (gri deschis)

        // Combinam
        vec3 lighting = ambient + diffuse + specular;

        // Aplicam ceata
        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);
        
        out_Color = vec4(mix(fogColor, lighting, f), 1.0);
    }
    
    // ------------------------------------
    // CAZ 4: SOARE (Billboarding)
    // ------------------------------------
    else if (codCol == 4)
    {
         out_Color = vec4(1.0, 1.0, 0.0, 1.0); // Galben simplu si curat
    }
    
    // ------------------------------------
    // CAZ 2, 3: CER, CROSSHAIR (Fara ceata)
    // ------------------------------------
    else if (codCol == 2 || codCol == 3)
    {
        out_Color = ex_Color; 
    }
    
    else if (codCol == 1)
    {
        // Afisam direct culoarea neagra setata in Vertex Shader (0.1, 0.1, 0.1, 0.5)
        // Fara sa o amestecam cu fogColor (albastru)
        out_Color = ex_Color; 
    }

    // ------------------------------------
    // CAZ 0: COPACI (Cu Ceata)
    // ------------------------------------
    else 
    {
        // Doar copacii si solul normal mai intra aici
        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);
        
        out_Color = vec4(mix(fogColor, ex_Color.rgb, f), ex_Color.a);
    }
}