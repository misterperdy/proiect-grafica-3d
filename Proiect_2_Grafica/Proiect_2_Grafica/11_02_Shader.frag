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

void main(void)
{
    // 1. Calculam distanta de la Camera la Pixelul curent
    float dist = distance(ex_ViewPos, ex_FragPos);

    // 2. Calculam factorul de ceata Liniar
    float f = (fogEnd - dist) / (fogEnd - fogStart);
    f = clamp(f, 0.0, 1.0);

    // 3. Amestecam Culoarea primita (ex_Color) cu Culoarea Cetii
    vec3 finalRGB = mix(fogColor, ex_Color.rgb, f);

    out_Color = vec4(finalRGB, ex_Color.a);
}