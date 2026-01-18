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

// Uniform pentru suprascriere culoare (licurici)
uniform vec4 overrideColor;

// Umbre (shadow mapping)
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Texturi 
uniform sampler2D diffuseMap; 
uniform sampler2D normalMap;
uniform sampler2D fireMap;

// Lumina punctiforma de la foc
uniform vec3 firePosFrag; 
uniform vec3 fireColorFrag; 

// --- LUMINI LICURICI ---
#define MAX_FIREFLY_LIGHTS 6
uniform vec3 fireflyPos[MAX_FIREFLY_LIGHTS];
uniform int numFirefliesActive; // cate pozitii sunt valide in array

// --- VOLUMETRIC FOG / GOD RAYS HELPER ---
float ComputeVolumetricFog(vec3 startPos, vec3 endPos)
{
    // Doar daca e zi sau apus, ca sa se vada razele soarelui
    // Noaptea ceata volumetrica e prea slaba.
    // Verificam luminozitatea luminii 
    
    vec3 rayDir = endPos - startPos;
    float rayLength = length(rayDir);
    rayDir /= rayLength; // normalize

    // Parametri de calitate
    const int STEPS = 15; // Numar de mostre pe raza (mai mare = mai fin, dar mai lent)
    float stepSize = rayLength / float(STEPS);
    
    // Dither pattern simplu pentru a ascunde artefactele de banding la numar mic de pasi
    float dither = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
    
    vec3 currentPos = startPos + rayDir * stepSize * dither;
    
    float accumFog = 0.0;

    for(int i = 0; i < STEPS; ++i)
    {
        // Transformam pozitia curenta din aer in spatiul umbrei
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(currentPos, 1.0);
        
        // Perspective divide
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;

        // Verificam daca e in Shadow Map
        // Daca e in afara hartii, consideram ca e lumina (1.0)
        float inLight = 1.0;
        if(projCoords.z <= 1.0 && projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0)
        {
            // Sample simplu (fara PCF aici pentru viteza)
            float depthVal = texture(shadowMap, projCoords.xy).r;
            // Bias mic
            if(projCoords.z - 0.005 > depthVal)
                inLight = 0.0; // E in umbra
        }

        // Acumulam lumina
        accumFog += inLight;
        
        currentPos += rayDir * stepSize;
        
        // Optimizare: Daca raza e prea lunga, ne oprim (ceata se vede doar aproape)
        // if(distance(startPos, currentPos) > 150.0) break;
    }

    // Normalizam rezultatul
    return (accumFog / float(STEPS));
}

float ShadowPCF(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // impartire perspectiva
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transformare in [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // daca e in afara frustum-ului luminii => fara umbra
    if (projCoords.z > 1.0) return 0.0;

    // bias pentru a reduce artefacte (shadow acne)
    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias = max(0.0012 * (1.0 - ndotl), 0.0004);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    // PCF 2x2 (silhouette mai "tare" vs 3x3)
    for (int x = -1; x <= 0; ++x)
    {
        for (int y = -1; y <= 0; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow *= 0.25;

    return shadow;
}

void main(void)
{
 
    // OBIECTE TEXTURATE (foc, banci) cu "bump" aproximat din textura de culoare
    if (codCol == 7)
    {
        vec3 base = texture(diffuseMap, ex_TexCoord).rgb;

        // 1. CALCUL NORMALA (standard vs bump)
        vec3 normal;
        
        // Daca avem o textura de normale legata, am folosi-o aici
        // Asta face ca lemnul/piatra sa aiba "relief" bazat pe cat de inchisa/deschisa e culoarea.
        
        // Calculam panta (gradientul) luminozitatii texturii
        float h_center = dot(base, vec3(0.333));
        float h_right  = dot(texture(diffuseMap, ex_TexCoord + vec2(0.005, 0.0)).rgb, vec3(0.333));
        float h_up     = dot(texture(diffuseMap, ex_TexCoord + vec2(0.0, 0.005)).rgb, vec3(0.333));
        
        // Reducem factorul de relief pentru un aspect mai natural (era 12.0)
        float dX = (h_center - h_right) * 3.5; 
        float dY = (h_center - h_up) * 3.5;

        // Perturbam normala geometrica
        // (Folosim dFdx/dFdy pentru a obtine normala suprafetei plate, apoi o rotim)
        vec3 pos_dx = dFdx(ex_FragPos);
        vec3 pos_dy = dFdy(ex_FragPos);
        vec3 geoNormal = normalize(cross(pos_dx, pos_dy));
        
        // Construim un cadru tangent (TBN) ad-hoc
        vec3 viewDir = normalize(ex_ViewPos - ex_FragPos);
        geoNormal = faceforward(geoNormal, -viewDir, geoNormal);
        
        // Aproximam Tangenta si Bitangenta
        vec3 T = normalize(pos_dx); 
        vec3 B = normalize(cross(geoNormal, T));
        
        // Noua normala "Bumped"
        normal = normalize(geoNormal + T * dX + B * dY);

        // -----------------------------------------------------
        // CONTINUARE ILUMINARE (LIGHTING)
        // -----------------------------------------------------

        // 1. LUMINA DIRECTIONALA (soare/luna)
        vec3 lightDir = normalize(lightPos - ex_FragPos);

        float ambientStrength = 0.28;
        vec3 ambient = ambientStrength * base;

        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * base;

        float spec = pow(max(dot(viewDir, reflect(-lightDir, normal)), 0.0), 24.0);
        vec3 specular = vec3(0.10) * spec;

        // 2. LUMINA PUNCTIFORMÃ (foc)
        float distToFire = distance(ex_FragPos, firePosFrag);
        // Atenuare: constanta + liniara + patratica
        // Adjust these values to control reach. 
        // 0.04 linear, 0.002 quadratic gives a range of roughly ~30-40 units.
        float att = 1.0 / (1.0 + 0.04 * distToFire + 0.002 * distToFire * distToFire);
        
        vec3 fireDir = normalize(firePosFrag - ex_FragPos);
        float fireNdotL = max(dot(normal, fireDir), 0.0);
        
        // Contributia focului (difuz)
        vec3 fireDiffuse = fireNdotL * fireColorFrag * att * base;

        // --- CONTRIBUTIE LICURICI (lumina subtila verde) ---
        vec3 ffLighting = vec3(0.0);
        if (numFirefliesActive > 0) 
        {
            vec3 ffColor = vec3(0.4, 0.9, 0.1); // Verde-lamaie
            for(int i = 0; i < numFirefliesActive; ++i)
            {
                float d = distance(ex_FragPos, fireflyPos[i]);
                // Raza mica (~4-5 unitati), cadere rapida
                float ffAtt = 1.0 / (1.0 + 2.0 * d + 5.0 * d * d); 
                ffLighting += ffColor * ffAtt * 1.5; // * intensity
            }
        }

        vec3 lighting = ambient + diffuse + specular + fireDiffuse + ffLighting;

        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);

        out_Color = vec4(mix(fogColor, lighting, f), 1.0);
        return;
    }

    // ------------------------------------
    // CAZ 6: FLACARA (flipbook) 
    // ------------------------------------
    if (codCol == 6)
    {
        vec4 tex = texture(fireMap, ex_TexCoord);

        float a = tex.a;
        float alpha = smoothstep(0.08, 0.35, a);
        if (alpha < 0.02) discard;

        // --- MODIFICARE INTENSITATE SI VIBRATIE ---
        vec3 vibrantColor = tex.rgb;

        // 1. Contrast
        vibrantColor = pow(vibrantColor, vec3(1.4));

        // 2. Boost (Intensitate): Inmultim culorile. Valorile > 1.0 vor fi saturate la maxim (alb stralucitor)
        vibrantColor *= 2.2; 

        // 3. Vibratie (Saturatie)
        // Inmultim Rosu si Verde mai mult decat Albastru
        vibrantColor *= vec3(1.2, 1.1, 0.8);

        out_Color = vec4(vibrantColor * ex_Color.rgb, alpha);
        return;
    }

    // ------------------------------------
    // CAZ 5: IARBA 
    // ------------------------------------
    if (codCol == 5)
    {
        vec3 color = texture(diffuseMap, ex_TexCoord).rgb;
        vec3 normal = texture(normalMap, ex_TexCoord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(ex_TBN * normal);

        vec3 ambient = 0.2 * color; 
        vec3 lightDir = normalize(lightPos - ex_FragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * color * 1.2; 

        vec3 viewDir = normalize(ex_ViewPos - ex_FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = vec3(0.3) * spec;

        vec3 lighting = ambient + diffuse + specular;

        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);
        out_Color = vec4(mix(fogColor, lighting, f), 1.0);
    }
    
    // ------------------------------------
    // CAZ 4: SOARE/LUNA
    // ------------------------------------
    else if (codCol == 4)
    {
         out_Color = ex_Color;
    }
    
    // ------------------------------------
    // CAZ 2, 3: CER, CROSSHAIR
    // ------------------------------------
    else if (codCol == 2 || codCol == 3)
    {
       
   
 
        if (codCol == 3) {
   
             out_Color = overrideColor; 
        } else {
             out_Color = ex_Color;
        }
    }
    
    else if (codCol == 1)
    {
        out_Color = ex_Color; 
    }

    // ------------------------------------
    // CAZ 8: GLOW PROCEDURAL (aura / pata de lumina)
    // ------------------------------------
    else if (codCol == 8)
    {
        vec2 center = vec2(0.5);
        float dist = distance(ex_TexCoord, center);

        // Disc moale pentru aura/pata de lumina
        float alpha = 1.0 - smoothstep(0.0, 0.5, dist);
        alpha = pow(alpha, 3.0);

        vec3 glowColor = vec3(1.0, 0.55, 0.22);
        out_Color = vec4(glowColor, alpha * 0.75);
        return;
    }

    // ------------------------------------
    // CAZ 0: OBIECTE ILUMINATE + SHADOW MAP
    // ------------------------------------
    else 
    {
       
        vec3 normal = normalize(ex_TBN[2]);
        if (length(normal) < 0.001)
        {
            vec3 dpdx = dFdx(ex_FragPos);
            vec3 dpdy = dFdy(ex_FragPos);
            normal = normalize(cross(dpdx, dpdy));
        }
        vec3 viewDir = normalize(ex_ViewPos - ex_FragPos);
        normal = faceforward(normal, -viewDir, normal);


        vec3 lightDir = normalize(lightPos - ex_FragPos);

        float shadow = ShadowPCF(lightSpaceMatrix * vec4(ex_FragPos, 1.0), normal, lightDir);

        
        float shadowStrength = 0.6;
        vec3 shaded = ex_Color.rgb * (1.0 - shadow * shadowStrength);

        float distToFire = length(ex_FragPos - firePosFrag);
        float attenuation = 1.0 / (1.0 + 0.04 * distToFire + 0.002 * distToFire * distToFire);
        vec3 fireDir = normalize(firePosFrag - ex_FragPos);
        float fireDiff = max(dot(normal, fireDir), 0.0);
        
        vec3 fireLighting = fireDiff * fireColorFrag * attenuation * 0.8; 
        
        // Mix it in nicely
        shaded += fireLighting;

        // --- CONTRIBUTIE LICURICI (pe teren/obiecte iluminate) ---
        if (numFirefliesActive > 0) 
        {
            vec3 ffLightingGround = vec3(0.0);
            vec3 ffColor = vec3(0.4, 0.9, 0.1); // Verde-lamaie
            for(int i = 0; i < numFirefliesActive; ++i)
            {
                float d = length(ex_FragPos - fireflyPos[i]);
                float ffAtt = 1.0 / (1.0 + 2.0 * d + 5.0 * d * d);
                ffLightingGround += ffColor * ffAtt * 1.5;
            }
            shaded += ffLightingGround; // Additive
        }

        // ----------------------------------------

        // --- VOLUMETRIC FOG (GOD RAYS) ---
        // Adaugam un strat de lumina volumetrica peste pixelul gata randat
        // Calculam cata lumina trece prin aer intre camera si obiect
        
        // Facem asta doar daca ceata e deschisa la culoare (zi/apus), nu noaptea
        if (fogColor.g > 0.15) 
        {
            // Limitare distanta pentru performanta
            float distToCam = length(ex_FragPos - ex_ViewPos);
            
            // Calculam intensitatea razelor (scade cu distanta)
            // Doar pe primii 150m
            if(distToCam < 180.0)
            {
                float scattering = ComputeVolumetricFog(ex_ViewPos, ex_FragPos);
                
                // Culoarea razelor (o derivam din culoarea cetii dar mai luminoasa/galbuie)
                vec3 rayColor = vec3(1.0, 0.95, 0.8) * 0.4; 
                if(fogColor.r > 0.7) rayColor = vec3(1.0, 0.6, 0.3) * 0.6; // Apus (mai portocaliu)

                // Blend additiv bazat pe cat de mult aer iluminat am traversat
                // scattering e 0.0 -> 1.0 (procentaj de pasi in lumina)
                // Inmultim cu distanta pentru ca mai mult aer = mai multa ceata
                float intensity = scattering * smoothstep(180.0, 0.0, distToCam) * 0.6; // 0.6 e puterea efectului
                
                shaded += rayColor * intensity;
            }
        }

        if (fogColor.g < 0.1) 
        {
            // conversie la luminanta (alb-negru)
            float lum = dot(shaded, vec3(0.2126, 0.7152, 0.0722));
            // amestecam cu o tenta albastruie pentru efect de noapte
            vec3 nightTint = vec3(lum) * vec3(0.6, 0.7, 1.0);
            shaded = mix(shaded, nightTint, 0.75);
        }

        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);

        out_Color = vec4(mix(fogColor, shaded, f), ex_Color.a);
    }
}