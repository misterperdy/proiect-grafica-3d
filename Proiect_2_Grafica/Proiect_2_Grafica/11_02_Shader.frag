#version 330 core

in vec4 ex_Color;      // Culoarea calculata in Vertex (Lumina sau Umbra)
in vec3 ex_FragPos;    // Pozitia exacta a pixelului in lume
in vec3 ex_ViewPos;    // Pozitia camerei

out vec4 out_Color;

uniform int codCol;

// Variabile Ceata (Le primim aici acum)
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;

uniform vec3 lightPos;

void main(void)
{
    if (codCol == 4)
    {
        // Deoarece folosim un Quad Billboard, nu mai avem nevoie de coordonate locale complexe.
        // Putem folosi un truc simplu: desenam un cerc pe patrat.
        // Dar daca vrei doar "Sfera Galbena" simpla, poti returna direct culoarea.
        
        // Varianta SIMPLA (Cerc perfect):
        // (Ignoram logica complicata, desenam doar galben)
        
        out_Color = vec4(1.0, 1.0, 0.0, 1.0); // Galben Pur
    }
    // CAZ 2 si 3: SKYBOX, NORI, CROSSHAIR (Fara ceata)
    else if (codCol == 2 || codCol == 3)
    {
        out_Color = ex_Color; 
    }
    // CAZ 0 si 1: SOL, COPACI, UMBRE (Cu Ceata)
    else 
    {
        float dist = distance(ex_ViewPos, ex_FragPos);
        float f = (fogEnd - dist) / (fogEnd - fogStart);
        f = clamp(f, 0.0, 1.0);

        vec3 finalRGB = mix(fogColor, ex_Color.rgb, f);
        out_Color = vec4(finalRGB, ex_Color.a);
    }
}